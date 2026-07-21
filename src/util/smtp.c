#include <gnutls/gnutls.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "smtp.h"

#define LOG_TAG "util.smtp"
#include "util/log.h"

/** Sizes for heap-allocated SMTP transaction buffers */
#define SMTP_BUF_SIZE 4096
#define SMTP_CMD_SIZE 2048
#define SMTP_MSG_SIZE 5120

/* ------------------------------------------------------------------ */
/* Base64 encoding (AUTH LOGIN)                                        */
/* ------------------------------------------------------------------ */

static const char b64_alphabet[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

/**
 * @brief Encode raw bytes into null-terminated Base64.
 * @param src   Input bytes.
 * @param len   Number of input bytes.
 * @param dst   Output buffer (allocated by caller, at least
 *              ((len + 2) / 3) * 4 + 1 bytes).
 */
static void b64_encode(const unsigned char* src, size_t len, char* dst) {
  size_t i = 0, j = 0;
  while (len >= 3) {
    dst[j++] = b64_alphabet[(src[0] & 0xFC) >> 2];
    dst[j++] = b64_alphabet[((src[0] & 0x03) << 4) | ((src[1] & 0xF0) >> 4)];
    dst[j++] = b64_alphabet[((src[1] & 0x0F) << 2) | ((src[2] & 0xC0) >> 6)];
    dst[j++] = b64_alphabet[src[2] & 0x3F];
    src += 3;
    len -= 3;
    i += 3;
  }
  (void)i;
  if (len > 0) {
    unsigned char tail[3] = {0};
    memcpy(tail, src, len);
    dst[j++] = b64_alphabet[(tail[0] & 0xFC) >> 2];
    dst[j++] = b64_alphabet[((tail[0] & 0x03) << 4) |
                            (len > 1 ? ((tail[1] & 0xF0) >> 4) : 0)];
    dst[j++] = (len > 1) ? b64_alphabet[((tail[1] & 0x0F) << 2)] : '=';
    dst[j++] = '=';
  }
  dst[j] = '\0';
}

/* ------------------------------------------------------------------ */
/* RFC 2822 date                                                        */
/* ------------------------------------------------------------------ */

static void fmt_rfc2822_date(char* buf, size_t bufsz, time_t t) {
  static const char* days[] = {"Sun", "Mon", "Tue", "Wed",
                               "Thu", "Fri", "Sat"};
  static const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
  struct tm tm_val;
  gmtime_r(&t, &tm_val);
  snprintf(buf, bufsz, "%s, %02d %s %04d %02d:%02d:%02d +0000",
           days[tm_val.tm_wday], tm_val.tm_mday,
           months[tm_val.tm_mon], tm_val.tm_year + 1900,
           tm_val.tm_hour, tm_val.tm_min, tm_val.tm_sec);
}

/* ------------------------------------------------------------------ */
/* TCP / DNS helpers                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Resolve hostname and connect a TCP socket.
 * @param host      Hostname or IP string.
 * @param port      Port number.
 * @param timeout_s Connection + recv timeout in seconds.
 * @return socket fd on success, -1 on failure (logged).
 */
static int tcp_connect(const char* host, uint16_t port, int timeout_s) {
  char port_str[8];
  snprintf(port_str, sizeof(port_str), "%u", port);

  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  struct addrinfo* result = NULL;
  int gaierr = getaddrinfo(host, port_str, &hints, &result);
  if (gaierr != 0) {
    LOG_E("DNS resolution failed for %s: %s", host, gai_strerror(gaierr));
    return -1;
  }

  int sockfd = -1;
  for (struct addrinfo* rp = result; rp; rp = rp->ai_next) {
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd < 0)
      continue;

    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) == 0)
      break;

    close(sockfd);
    sockfd = -1;
  }

  freeaddrinfo(result);

  if (sockfd < 0) {
    LOG_E("TCP connect to %s:%u failed", host, port);
    return -1;
  }

  /* Set receive timeout */
  struct timeval tv = {.tv_sec = timeout_s, .tv_usec = 0};
  if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    LOG_W("failed to set SO_RCVTIMEO on socket");

  return sockfd;
}

/* ------------------------------------------------------------------ */
/* GnuTLS helpers                                                       */
/* ------------------------------------------------------------------ */

static bool gnutls_global_inited = false;

/**
 * @brief Lazily initialise GnuTLS global state.
 */
static void gnutls_ensure_global_init(void) {
  if (!gnutls_global_inited) {
    gnutls_global_init();
    gnutls_global_inited = true;
  }
}

/* ------------------------------------------------------------------ */
/* SMTP state machine — single connection                               */
/* ------------------------------------------------------------------ */

/** SMTP transaction steps */
typedef enum {
  SMTP_STEP_INIT = 0,
  SMTP_STEP_EHLO,
  SMTP_STEP_AUTH_LOGIN,
  SMTP_STEP_USER,
  SMTP_STEP_PASS,
  SMTP_STEP_MAIL_FROM,
  SMTP_STEP_RCPT_TO,
  SMTP_STEP_DATA,
  SMTP_STEP_BODY,
  SMTP_STEP_QUIT,
  SMTP_STEP_DONE,
  SMTP_STEP_ERROR
} smtp_step_t;

static const char* smtp_step_name(smtp_step_t s) {
  switch (s) {
  case SMTP_STEP_INIT:       return "INIT";
  case SMTP_STEP_EHLO:       return "EHLO";
  case SMTP_STEP_AUTH_LOGIN: return "AUTH_LOGIN";
  case SMTP_STEP_USER:       return "USER";
  case SMTP_STEP_PASS:       return "PASS";
  case SMTP_STEP_MAIL_FROM:  return "MAIL_FROM";
  case SMTP_STEP_RCPT_TO:    return "RCPT_TO";
  case SMTP_STEP_DATA:       return "DATA";
  case SMTP_STEP_BODY:       return "BODY";
  case SMTP_STEP_QUIT:       return "QUIT";
  case SMTP_STEP_DONE:       return "DONE";
  default:                   return "?";
  }
}

#if LOG_DEBUG_ENABLED

static char* smtp_chomp(char* s) {
  size_t len = strlen(s);
  while (len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n'))
    s[--len] = '\0';
  return s;
}

#endif /* LOG_DEBUG_ENABLED */

/**
 * @brief Execute a complete SMTP transaction over an established
 *        GnuTLS session.
 *
 * @return true if the email was accepted by the server, false otherwise.
 */
static bool smtp_transaction(gnutls_session_t session,
                             const smtp_config_t* cfg,
                             const char** to_list,
                             const char* subject,
                             const char* body,
                             smtp_session_t* sess) {
  bool ok = false;
  char* buf = sess->buf;
  char* cmd = sess->cmd;
  char* msg = NULL;

  memset(buf, 0, SMTP_BUF_SIZE);

  smtp_step_t step = SMTP_STEP_INIT;
  int buf_pos = 0;

  /* Reusable: encode credentials once */
  char b64_user[256];
  char b64_pass[256];
  b64_encode((const unsigned char*)cfg->username, strlen(cfg->username),
             b64_user);
  b64_encode((const unsigned char*)cfg->password, strlen(cfg->password),
             b64_pass);

  /* Build RFC 2822 date */
  char date_buf[64];
  fmt_rfc2822_date(date_buf, sizeof(date_buf), time(NULL));

  const char* mime_type =
      (cfg->content_type == 1) ? "text/plain" : "text/html";

  LOG_D("SMTP >> %s:%u from=%s to=%s", cfg->server, cfg->port,
        cfg->from, to_list[0]);

  while (step < SMTP_STEP_DONE) {
    /* --- Read server response --- */
    ssize_t n = gnutls_record_recv(session, buf + buf_pos,
                                   SMTP_BUF_SIZE - buf_pos - 1);
    if (n <= 0) {
      if (buf_pos > 0)
        LOG_D("S[%s]: %s (partial)", smtp_step_name(step), smtp_chomp(buf));
      if (n == GNUTLS_E_TIMEDOUT || n == GNUTLS_E_AGAIN)
        LOG_E("SMTP read timed out at step %s", smtp_step_name(step));
      else if (n == 0)
        LOG_E("SMTP server closed connection at step %s",
              smtp_step_name(step));
      else
        LOG_E("SMTP read error at step %s: %s",
              smtp_step_name(step), gnutls_strerror((int)n));
      goto cleanup;
    }

    buf_pos += (int)n;
    buf[buf_pos] = '\0';
    LOG_D("S[%s]: %s", smtp_step_name(step), smtp_chomp(buf));

    /* --- State transitions --- */
    switch (step) {
    case SMTP_STEP_INIT:
      if (strstr(buf, "220")) {
        LOG_D("C[%s]: EHLO localhost", smtp_step_name(step));
        gnutls_record_send(session, "EHLO localhost\r\n", 16);
        step = SMTP_STEP_EHLO;
        buf_pos = 0;
        buf[0] = '\0';
      }
      break;

    case SMTP_STEP_EHLO:
      if (strstr(buf, "250 ")) {
        if (cfg->username && *cfg->username) {
          LOG_D("C[%s]: AUTH LOGIN", smtp_step_name(step));
          gnutls_record_send(session, "AUTH LOGIN\r\n", 12);
          step = SMTP_STEP_AUTH_LOGIN;
        } else {
          LOG_D("C[%s]: MAIL FROM:<%s> (no auth)", smtp_step_name(step),
                cfg->from);
          snprintf(cmd, SMTP_CMD_SIZE, "MAIL FROM:<%s>\r\n", cfg->from);
          gnutls_record_send(session, cmd, strlen(cmd));
          step = SMTP_STEP_MAIL_FROM;
        }
        buf_pos = 0;
        buf[0] = '\0';
      }
      break;

    case SMTP_STEP_AUTH_LOGIN:
      if (strstr(buf, "334")) {
        LOG_D("C[%s]: %s", smtp_step_name(step), b64_user);
        snprintf(cmd, SMTP_CMD_SIZE, "%s\r\n", b64_user);
        gnutls_record_send(session, cmd, strlen(cmd));
        step = SMTP_STEP_USER;
        buf_pos = 0;
        buf[0] = '\0';
      }
      break;

    case SMTP_STEP_USER:
      if (strstr(buf, "334")) {
        LOG_D("C[%s]: **** (base64 pass masked)", smtp_step_name(step));
        snprintf(cmd, SMTP_CMD_SIZE, "%s\r\n", b64_pass);
        gnutls_record_send(session, cmd, strlen(cmd));
        step = SMTP_STEP_PASS;
        buf_pos = 0;
        buf[0] = '\0';
      }
      break;

    case SMTP_STEP_PASS:
      if (strstr(buf, "235")) {
        LOG_D("C[%s]: MAIL FROM:<%s>", smtp_step_name(step), cfg->from);
        snprintf(cmd, SMTP_CMD_SIZE, "MAIL FROM:<%s>\r\n", cfg->from);
        gnutls_record_send(session, cmd, strlen(cmd));
        step = SMTP_STEP_MAIL_FROM;
        buf_pos = 0;
        buf[0] = '\0';
      } else if (strstr(buf, "535")) {
        LOG_E("SMTP authentication failed — check username/password");
        goto cleanup;
      }
      break;

    case SMTP_STEP_MAIL_FROM:
      if (strstr(buf, "250 ")) {
        LOG_D("C[%s]: RCPT TO:<%s>", smtp_step_name(step), to_list[0]);
        snprintf(cmd, SMTP_CMD_SIZE, "RCPT TO:<%s>\r\n", to_list[0]);
        gnutls_record_send(session, cmd, strlen(cmd));
        step = SMTP_STEP_RCPT_TO;
        buf_pos = 0;
        buf[0] = '\0';
      }
      break;

    case SMTP_STEP_RCPT_TO:
      if (strstr(buf, "250 ")) {
        LOG_D("C[%s]: DATA", smtp_step_name(step));
        gnutls_record_send(session, "DATA\r\n", 6);
        step = SMTP_STEP_DATA;
        buf_pos = 0;
        buf[0] = '\0';
      }
      break;

    case SMTP_STEP_DATA:
      if (strstr(buf, "354")) {
        LOG_D("C[%s]: <headers+body+.> subj='%s' body=%zub",
              smtp_step_name(step), subject, strlen(body));
        /* Build full RFC 2822 message */
        msg = sess->msg;

        int hdr_len = snprintf(msg, SMTP_MSG_SIZE,
                               "From: %s\r\n"
                               "To: %s",
                               cfg->from, to_list[0]);
        if (hdr_len < 0 || (size_t)hdr_len >= SMTP_MSG_SIZE) {
          LOG_E("message header too long");
          goto cleanup;
        }

        /* Append additional recipients to To header */
        for (int i = 1; to_list[i]; i++) {
          int rem = (int)(SMTP_MSG_SIZE - hdr_len);
          int w = snprintf(msg + hdr_len, rem, ", %s", to_list[i]);
          if (w < 0 || w >= rem)
            break;
          hdr_len += w;
        }

        int rem = (int)(SMTP_MSG_SIZE - hdr_len);
        snprintf(msg + hdr_len, rem,
                 "\r\n"
                 "Subject: %s\r\n"
                 "Date: %s\r\n"
                 "Content-Type: %s; charset=UTF-8\r\n"
                 "\r\n",
                 subject, date_buf, mime_type);

        /* Send headers + body + end-of-data marker */
        gnutls_record_send(session, msg, strlen(msg));
        gnutls_record_send(session, body, strlen(body));
        gnutls_record_send(session, "\r\n.\r\n", 5);

        step = SMTP_STEP_BODY;
        buf_pos = 0;
        buf[0] = '\0';
      }
      break;

    case SMTP_STEP_BODY:
      if (strstr(buf, "250 ")) {
        LOG_D("C[%s]: QUIT", smtp_step_name(step));
        gnutls_record_send(session, "QUIT\r\n", 6);
        step = SMTP_STEP_QUIT;
        buf_pos = 0;
        buf[0] = '\0';
      }
      break;

    case SMTP_STEP_QUIT:
      if (strstr(buf, "221")) {
        LOG_D("SMTP transaction complete");
        step = SMTP_STEP_DONE;
      }
      break;

    default:
      LOG_E("unexpected SMTP state %d", step);
      goto cleanup;
    }
  }

  ok = (step == SMTP_STEP_DONE);

cleanup:
  if (!ok)
    LOG_D("SMTP transaction FAILED at step %s", smtp_step_name(step));
  return ok;
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void smtp_config_init(smtp_config_t* cfg) {
  memset(cfg, 0, sizeof(*cfg));
  cfg->port = 465;
  cfg->content_type = 2;
  cfg->timeout_sec = 30;
}

void smtp_config_dispose(smtp_config_t* cfg) {
  free(cfg->server);
  free(cfg->username);
  free(cfg->password);
  free(cfg->from);
  memset(cfg, 0, sizeof(*cfg));
}

void smtp_session_init(smtp_session_t* sess) {
  memset(sess, 0, sizeof(*sess));

  gnutls_ensure_global_init();

  int rc = gnutls_certificate_allocate_credentials(&sess->xcred);
  if (rc < 0) {
    LOG_E("gnutls_certificate_allocate_credentials: %s", gnutls_strerror(rc));
    return;
  }

  /* Load system CA trust store */
  int ca_count = gnutls_certificate_set_x509_system_trust(sess->xcred);
  if (ca_count <= 0) {
    LOG_W("gnutls_certificate_set_x509_system_trust returned %d, "
          "trying file fallback",
          ca_count);
    ca_count = gnutls_certificate_set_x509_trust_file(
        sess->xcred, "/etc/ssl/certs/ca-certificates.crt", GNUTLS_X509_FMT_PEM);
    if (ca_count <= 0)
      LOG_W("no CA certificates loaded — TLS verification may fail");
  }

  if (ca_count > 0)
    LOG_I("loaded %d CA certificate(s)", ca_count);

  /* Allocate reusable transaction buffers */
  sess->buf = malloc(SMTP_BUF_SIZE);
  sess->cmd = malloc(SMTP_CMD_SIZE);
  sess->msg = malloc(SMTP_MSG_SIZE);
  if (!sess->buf || !sess->cmd || !sess->msg) {
    LOG_E("failed to allocate SMTP session buffers");
    free(sess->buf);
    free(sess->cmd);
    free(sess->msg);
    sess->buf = sess->cmd = sess->msg = NULL;
    return;
  }

  sess->xcred_valid = true;
}

void smtp_session_dispose(smtp_session_t* sess) {
  if (sess->session_data.data) {
    gnutls_free(sess->session_data.data);
    sess->session_data.data = NULL;
    sess->session_data.size = 0;
  }
  if (sess->xcred_valid) {
    gnutls_certificate_free_credentials(sess->xcred);
    sess->xcred_valid = false;
  }
  free(sess->buf);
  free(sess->cmd);
  free(sess->msg);
  sess->buf = sess->cmd = sess->msg = NULL;
}

bool smtp_send(const smtp_config_t* cfg,
               smtp_session_t* sess,
               const char** to_list,
               const char* subject,
               const char* body) {
  if (!cfg || !cfg->server || !*cfg->server || !cfg->from || !*cfg->from ||
      !to_list || !to_list[0] || !subject || !body) {
    LOG_E("invalid smtp_send arguments");
    return false;
  }

  if (!sess || !sess->xcred_valid || !sess->buf) {
    LOG_E("smtp_session not initialised");
    return false;
  }

  /* 1. TCP connect */
  int sockfd = tcp_connect(cfg->server, cfg->port, cfg->timeout_sec);
  if (sockfd < 0)
    return false;

  /* 2. GnuTLS session setup */
  gnutls_session_t tls = NULL;
  int rc = gnutls_init(&tls, GNUTLS_CLIENT);
  if (rc < 0) {
    LOG_E("gnutls_init: %s", gnutls_strerror(rc));
    close(sockfd);
    return false;
  }

  /* Force TLS 1.2 — GnuTLS 3.6.4 TLS 1.3 has compatibility issues
     with some SMTP servers (e.g. post-handshake message handling). */
  rc = gnutls_priority_set_direct(tls, "NORMAL:-VERS-TLS1.3", NULL);
  if (rc < 0)
    LOG_W("gnutls_priority_set_direct: %s", gnutls_strerror(rc));
  gnutls_credentials_set(tls, GNUTLS_CRD_CERTIFICATE, sess->xcred);
  gnutls_server_name_set(tls, GNUTLS_NAME_DNS, cfg->server,
                         strlen(cfg->server));
  gnutls_session_set_verify_cert(tls, cfg->server, 0);

  /* Session resumption: inject cached session data before handshake */
  if (sess->session_data.size > 0) {
    LOG_D("injecting cached TLS session (%u bytes)", sess->session_data.size);
    gnutls_session_set_data(tls, sess->session_data.data,
                            sess->session_data.size);
  }

  gnutls_transport_set_int(tls, sockfd);

  /* 3. TLS handshake */
  rc = gnutls_handshake(tls);
  if (rc < 0) {
    LOG_E("TLS handshake: %s", gnutls_strerror(rc));
    gnutls_deinit(tls);
    close(sockfd);
    return false;
  }

  if (gnutls_session_is_resumed(tls)) {
    LOG_I("TLS session resumed — fast handshake");
  } else {
#if LOG_DEBUG_ENABLED
    gnutls_protocol_t ver = gnutls_protocol_get_version(tls);
    gnutls_cipher_algorithm_t cipher = gnutls_cipher_get(tls);
    gnutls_kx_algorithm_t kx = gnutls_kx_get(tls);
    size_t pending = gnutls_record_check_pending(tls);
    LOG_D("full TLS handshake: %s-%s-%s, pending=%zub",
          gnutls_protocol_get_name(ver),
          gnutls_kx_get_name(kx),
          gnutls_cipher_get_name(cipher),
          pending);
#endif

    /* Cache the new session data for future resumption */
    gnutls_datum_t new_data = {NULL, 0};
    rc = gnutls_session_get_data2(tls, &new_data);
    if (rc == 0 && new_data.size > 0) {
      if (sess->session_data.data)
        gnutls_free(sess->session_data.data);
      sess->session_data = new_data;
      LOG_D("cached TLS session (%u bytes)", new_data.size);
    }
  }

  /* 4. SMTP transaction */
  bool ok = smtp_transaction(tls, cfg, to_list, subject, body, sess);

  if (ok)
    LOG_D("email sent to %s, subject='%s'", to_list[0], subject);
  else
    LOG_E("SMTP transaction failed");

  /* 5. Cleanup */
  gnutls_bye(tls, GNUTLS_SHUT_RDWR);
  gnutls_deinit(tls);
  close(sockfd);

  return ok;
}
