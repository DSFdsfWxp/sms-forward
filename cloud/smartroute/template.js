function escapeHtml(str) {
  if (!str) return '';
  return str.replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function dateStr(ts) {
  if (!ts) return '';
  return new Date(ts * 1000).toLocaleString('zh-CN', {
    timeZone: 'Asia/Shanghai'
  });
}

function recordVars(record) {
  const totalCount = record.total_count || process.env.TOTAL_COUNT || '0';
  const maxCount = record.max_count || process.env.MAX_COUNT || '0';
  const inboxCount = record.inbox_count || process.env.INBOX_COUNT || '0';
  return {
    phone: (record && record.phone) || '',
    contacts: (record && record.contacts) || '',
    content: (record && record.content) || '',
    timestamp: (record && record.timestamp !== undefined)
      ? String(record.timestamp) : '0',
    datetime: (record && record.timestamp)
      ? dateStr(record.timestamp) : '',
    inbox_count: String(inboxCount),
    unread_count: '0',
    outbox_count: '0',
    draft_count: '0',
    total_count: String(totalCount),
    max_count: String(maxCount),
  };
}

function applyTemplate(tmpl, record, contentType) {
  const isHtml = contentType === 2;
  const vars = recordVars(record);
  let result = '';
  let i = 0;
  while (i < tmpl.length) {
    if (tmpl[i] === '{' && tmpl[i + 1] === '{') {
      result += '{';
      i += 2;
    } else if (tmpl[i] === '}' && tmpl[i + 1] === '}') {
      result += '}';
      i += 2;
    } else if (tmpl[i] === '{') {
      const end = tmpl.indexOf('}', i + 1);
      if (end === -1) {
        result += '{';
        i++;
      } else {
        const key = tmpl.slice(i + 1, end);
        if (vars.hasOwnProperty(key)) {
          result += isHtml ? escapeHtml(vars[key]) : vars[key];
        } else {
          result += '{' + key + '}';
        }
        i = end + 1;
      }
    } else {
      result += tmpl[i];
      i++;
    }
  }
  return result;
}

module.exports = { escapeHtml, applyTemplate };
