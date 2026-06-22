const nodemailer = require('nodemailer');
const { applyTemplate, escapeHtml } = require('./template');

let transporter = null;

function getTransporter() {
  if (!transporter) {
    transporter = nodemailer.createTransport({
      host: process.env.SMTP_HOST,
      port: parseInt(process.env.SMTP_PORT || '465', 10),
      secure: parseInt(process.env.SMTP_PORT || '465', 10) === 465,
      auth: {
        user: process.env.SMTP_USER,
        pass: process.env.SMTP_PASS,
      },
    });
  }
  return transporter;
}

function mailSubject(record) {
  const isAlert = record && record.type === 'alert';
  if (isAlert) return '[SMS Alert] 短信即将存满';
  return '[SMS] ' + (record.contacts || record.phone || '未知号码');
}

function mailHtml(record) {
  const isAlert = record && record.type === 'alert';
  if (isAlert) {
    const tpl = process.env.TEMPLATE_EMAIL_ALERT ||
      '<h2>⚠️ 短信即将存满</h2><p>已用: <b>{total_count}</b> / <b>{max_count}</b></p>';
    return applyTemplate(tpl, record, 2);
  }
  const tpl = process.env.TEMPLATE_EMAIL ||
    '<h2>📩 新短信</h2>'
    + '<table>'
    + '<tr><td><b>发件人</b></td><td>{contacts}</td></tr>'
    + '<tr><td><b>号码</b></td><td>{phone}</td></tr>'
    + '<tr><td><b>时间</b></td><td>{datetime}</td></tr>'
    + '</table>'
    + '<hr><pre style="font-size:14px;white-space:pre-wrap">{content}</pre>';
  return applyTemplate(tpl, record, 2);
}

function mailText(record) {
  const isAlert = record && record.type === 'alert';
  if (isAlert) {
    const tpl = process.env.TEMPLATE_EMAIL_ALERT ||
      'SMS storage almost full!\nUsed: {total_count} / {max_count}';
    return applyTemplate(tpl, record, 1);
  }
  const tpl = process.env.TEMPLATE_EMAIL ||
    'From: {contacts} ({phone})\nTime: {datetime}\n\n{content}';
  return applyTemplate(tpl, record, 1);
}

async function sendMail(record) {
  await getTransporter().sendMail({
    from: process.env.SMTP_USER,
    to: process.env.SMTP_TO,
    subject: mailSubject(record),
    text: mailText(record),
    html: mailHtml(record),
  });
}

module.exports = { sendMail };
