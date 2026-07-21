# Route 路由后端

根据关键字和正则表达式将短信分发到两个子后端。匹配的短信发往 matchedBackend，不匹配的发往 unmatchedBackend；告警任务同时发往两个后端。

## 配置

```json
{
  "push": {
    "backendName": "route",
    "route": {
      "matchedBackend": "smtp",
      "unmatchedBackend": "wxpusher",
      "keywords": ["验证码", "快递"],
      "regexPatterns": ["您的.*验证码.*"],
      "matchField": "content"
    }
  }
}
```

### 配置项

| 键 | 类型 | 默认值 | 说明 |
|----|------|--------|------|
| `matchedBackend` | string | `""` | 匹配时路由到的后端名称（须在 `push_backends` 注册表中存在） |
| `unmatchedBackend` | string | `""` | 不匹配时路由到的后端名称 |
| `keywords` | string[] | `[]` | 关键字列表，大小写不敏感子串匹配 |
| `regexPatterns` | string[] | `[]` | POSIX 扩展正则表达式列表，最多 32 个 |
| `matchField` | string | `"content"` | 匹配字段：`"content"`、`"phone"` 或 `"contacts"` |

### 匹配逻辑

1. 对每条短信，取出 `matchField` 指定字段的文本
2. 若文本包含任一关键字（大小写不敏感），视为匹配
3. 若文本匹配任一正则表达式，视为匹配
4. 匹配 → 发往 `matchedBackend`；不匹配 → 发往 `unmatchedBackend`
5. 告警任务（`push_type_alert_smsbox_almost_full`）不受路由规则影响，同时发往两个后端

### 推送间隔

`min_interval_ms` 动态取 `matchedBackend` 和 `unmatchedBackend` 两者 `min_interval_ms` 的最大值，确保不超出任一子后端的频率限制。

### 子后端配置

`matchedBackend` 和 `unmatchedBackend` 所指向的后端各自从 `push.<name>.*` 命名空间下读取自己的配置，与直接使用时的配置完全一致。例如：

```json
{
  "push": {
    "backendName": "route",
    "route": {
      "matchedBackend": "smtp",
      "unmatchedBackend": "wxpusher",
      "keywords": ["验证码"],
      "matchField": "content"
    },
    "smtp": {
      "server": "smtp.example.com",
      "port": 587,
      "useTls": true,
      "username": "alert@example.com",
      "password": "xxx",
      "from": "alert@example.com",
      "to": ["admin@example.com"]
    },
    "wxpusher": {
      "mode": "standard",
      "appToken": "AT_xxx",
      "uids": ["UID_xxx"]
    }
  }
}
```

## 示例：敏感短信邮件推送，普通短信微信推送

```json
{
  "push": {
    "backendName": "route",
    "minPushIntervalMs": 2000,
    "route": {
      "matchedBackend": "smtp",
      "unmatchedBackend": "wxpusher",
      "keywords": ["验证码", "校验码", "动态码", "银行卡"],
      "regexPatterns": ["【.*】.*验证码", "尾号\\d{4}"],
      "matchField": "content"
    },
    "smtp": {
      "server": "smtp.qq.com",
      "port": 587,
      "useTls": true,
      "username": "sender@qq.com",
      "password": "auth-code",
      "from": "sender@qq.com",
      "to": ["admin@company.com"],
      "contentType": 2,
      "minIntervalMs": 10000
    },
    "wxpusher": {
      "mode": "spt",
      "spt": "SPT_normal",
      "contentType": 2,
      "template_msg": "<b>{contacts}</b>: {content}"
    }
  }
}
```

此例中，验证码、银行卡等敏感短信通过 SMTP 邮件安全送达，普通短信通过 WxPusher 微信即时推送。
