const { decrypt } = require('./decrypt');
const { detect } = require('./detector');
const { sendMail } = require('./mailer');
const { sendMsg } = require('./wxpusher');

function auth(event) {
  const token = process.env.TOKEN;
  if (!token) return true;
  const header = (event.headers || {})['Authorization']
    || (event.headers || {})['authorization'] || '';
  return header === 'Bearer ' + token || header === token;
}

exports.handler = async (event, context) => {
  try {
    if (!auth(event))
      return { statusCode: 401, body: 'unauthorized' };

    const encryptedBase64 = (event.body || '').trim();
    if (!encryptedBase64)
      return { statusCode: 400, body: 'empty body' };

    const records = decrypt(encryptedBase64,
      process.env.AES_KEY, process.env.AES_IV);
    const list = Array.isArray(records) ? records : [records];

    for (const record of list) {
      const type = detect(record.content || '');
      if (type) {
        await sendMail(record);
      } else {
        await sendMsg(record);
      }
    }

    return { statusCode: 200, body: 'ok' };
  } catch (err) {
    console.error('smartroute error:', err);
    return { statusCode: 500, body: err.message };
  }
};
