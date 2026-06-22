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
  event = JSON.parse(event);
  if (event.isBase64Encoded) {
    event.body = Buffer.from(event.body, 'base64').toString('utf8');
    event.isBase64Encoded = false;
  }

  try {
    if (!auth(event)) {
      console.warn('unauthorized', event);
      return { statusCode: 401, body: 'unauthorized' };
    }

    const encryptedBase64 = (event.body || '').trim();
    if (!encryptedBase64) {
      console.warn('empty body', event);
      return { statusCode: 400, body: 'empty body' };
    }

    const records = decrypt(encryptedBase64,
      process.env.AES_KEY, process.env.AES_IV);
    const list = Array.isArray(records) ? records : [records];
    console.log('records count:', list.length)

    let normalRecords = [];
    for (const record of list) {
      if (!record.contacts) {
        let content = record.content.trim();
        if (content.startsWith('【') && content.includes('】'))
          record.contacts = content.substring(1, content.indexOf('】'));
      }
      const filtered = detect(record.content || '');
      if (filtered) {
        console.log(`record from ${record.phone} => mail`);
        await sendMail(record);
      } else {
        console.log(`record from ${record.phone} => wxpusher`);
        normalRecords.push(record);
      }
    }
    if (normalRecords.length > 0)
      await sendMsg(normalRecords);

    return { statusCode: 200, body: 'ok' };
  } catch (err) {
    console.error('smartroute error:', err, event);
    return { statusCode: 500, body: err.message };
  }
};
