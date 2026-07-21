#pragma once

#include <gnutls/gnutls.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief SMTP connection configuration.
 *
 * All string fields are owned by this struct; caller must use
 * smtp_config_dispose() to free them.
 */
typedef struct {
  char* server;      /**< SMTP server hostname, e.g. "smtp.example.com" */
  uint16_t port;     /**< Server port (default 465 for implicit TLS / SMTPS) */
  char* username;    /**< AUTH LOGIN username, NULL to skip authentication */
  char* password;    /**< AUTH LOGIN password */
  char* from;        /**< Sender email address */
  int content_type;  /**< 1=text/plain, 2=text/html (default 2) */
  int timeout_sec;   /**< Connection timeout in seconds */
} smtp_config_t;

/**
 * @brief Persistent GnuTLS session state for TLS session resumption.
 *
 * Created once per smtp backend lifetime, shared across smtp_send() calls.
 * On first connection a full TLS handshake is performed and session data
 * is cached; subsequent connections resume the session, skipping
 * certificate verification and key exchange.
 */
typedef struct {
  gnutls_certificate_credentials_t xcred;  /**< Global GnuTLS X.509 credentials */
  gnutls_datum_t session_data;             /**< Cached TLS session data */
  bool xcred_valid;                        /**< true once xcred is allocated */
  char* buf;                               /**< SMTP response buffer (4096 B) */
  char* cmd;                               /**< SMTP command buffer (2048 B) */
  char* msg;                               /**< RFC 2822 message buffer (5120 B) */
} smtp_session_t;

/**
 * @brief Initialise an smtp_config_t with safe defaults.
 *        Sets port to 465 (SMTPS), content_type to 2 (HTML),
 *        timeout_sec to 30.
 */
void smtp_config_init(smtp_config_t* cfg);

/**
 * @brief Free all heap-allocated strings inside the config.
 *        Safe to call with zero-initialised struct.
 */
void smtp_config_dispose(smtp_config_t* cfg);

/**
 * @brief Initialise GnuTLS global state for SMTP sessions.
 *
 *        Allocates X.509 credentials and loads system CA trust store.
 *        Must be called once before smtp_send(); call smtp_session_dispose()
 *        to release resources.
 *
 * @param sess  Uninitialised smtp_session_t.
 */
void smtp_session_init(smtp_session_t* sess);

/**
 * @brief Release GnuTLS resources held by the session.
 *        Safe to call with zero-initialised struct.
 */
void smtp_session_dispose(smtp_session_t* sess);

/**
 * @brief Send an email via SMTP over implicit TLS (SMTPS).
 *
 *        Opens a raw TCP socket to cfg->server:cfg->port, performs a
 *        GnuTLS handshake (with optional session resumption via sess),
 *        and executes the SMTP transaction:
 *          EHLO → AUTH LOGIN → MAIL FROM → RCPT TO → DATA → QUIT.
 *
 *        Constructs an RFC 2822 message with From, To, Subject, Date
 *        headers.  Body is sent as-is (caller is responsible for
 *        providing correctly formatted content per content_type).
 *
 * @param cfg       SMTP server configuration.
 * @param sess      Persistent session for TLS session resumption.
 * @param to_list   NULL-terminated array of recipient addresses.
 * @param subject   Email subject line (must not be NULL).
 * @param body      Email body text (must not be NULL).
 * @return true on success, false on any error (logged internally).
 */
bool smtp_send(const smtp_config_t* cfg,
               smtp_session_t* sess,
               const char** to_list,
               const char* subject,
               const char* body);
