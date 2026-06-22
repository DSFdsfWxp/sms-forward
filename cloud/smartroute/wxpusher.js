const https = require('https');
const { applyTemplate } = require('./template');

function envList(key) {
  return (process.env[key] || '').split(',').filter(Boolean);
}

function buildRequest(record) {
  const mode = process.env.WXPUSHER_MODE || 'standard';
  const isAlert = (record && record.type === 'alert');
  const defaultMsg = '<b>新短信</b><br>{phone} ({contacts})<br><pre>{content}</pre>';
  const defaultAlert = '<b>短信即将存满！</b><br>已用: {total_count}/{max_count}';
  const template = isAlert
    ? (process.env.TEMPLATE_ALERT || defaultAlert)
    : (process.env.TEMPLATE_MSG || defaultMsg);
  const contentType = parseInt(process.env.CONTENT_TYPE || '2', 10);
  const summary = process.env.SUMMARY || '';
  const url = process.env.URL || '';

  const body = {
    content: applyTemplate(template, record, contentType),
    contentType: contentType,
  };
  if (summary) body.summary = summary;
  if (url) body.url = url;

  if (mode === 'spt') {
    body.spt = process.env.WXPUSHER_SPT || '';
    const sptList = envList('WXPUSHER_SPT_LIST');
    if (sptList.length) body.sptList = sptList;
    return { url: 'https://wxpusher.zjiecode.com/api/send/message/simple-push', body };
  }

  body.appToken = process.env.WXPUSHER_APP_TOKEN || '';
  const uids = envList('WXPUSHER_UIDS');
  if (uids.length) body.uids = uids;
  const topicIds = envList('WXPUSHER_TOPIC_IDS').map(Number);
  if (topicIds.length) body.topicIds = topicIds;
  return { url: 'https://wxpusher.zjiecode.com/api/send/message', body };
}

function send(request) {
  return new Promise((resolve, reject) => {
    const bodyStr = JSON.stringify(request.body);
    const req = https.request(request.url, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Content-Length': Buffer.byteLength(bodyStr),
      },
    }, (res) => {
      let data = '';
      res.on('data', chunk => { data += chunk; });
      res.on('end', () => {
        if (res.statusCode >= 200 && res.statusCode < 300)
          resolve(data);
        else
          reject(new Error('WxPusher ' + res.statusCode + ': ' + data));
      });
    });
    req.on('error', reject);
    req.write(bodyStr);
    req.end();
  });
}

async function sendMsg(record) {
  return send(buildRequest(record));
}

module.exports = { sendMsg };
