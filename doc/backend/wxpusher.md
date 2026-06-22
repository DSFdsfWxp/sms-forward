
# WxPusher 后端

推送至 [WxPusher](https://wxpusher.zjiecode.com) 平台，支持标准推送和极简推送 (SPT) 两种模式。

## 配置

```json
{
  "push": {
    "backendName": "wxpusher",
    "minPushIntervalMs": 5000,
    "wxpusher": {
      "mode": "standard",
      "appToken": "AT_xxxxxxxxxxxxxxxxxxxx",
      "uids": ["UID_xxxxxxxxxxxxxxxxxxx"],
      "topicIds": [],
      "spt": "",
      "sptList": [],
      "contentType": 2,
      "summary": "",
      "url": "",
      "template_msg": "",
      "template_alert": "",
      "separator": "\n---\n"
    }
  }
}
```

### 通用

| 键 | 类型 | 默认值 | 说明 |
|-----|------|---------|------|
| `mode` | string | `"standard"` | `"standard"` 或 `"spt"` |
| `contentType` | int | `2` | 推出消息类型 1=纯文本, 2=html, 3=markdown |
| `summary` | string | `""` | 推送摘要（最长 100 字符） |
| `url` | string | `""` | 消息点击跳转链接 |
| `template_msg` | string | `"<h2>新短信</h2><pre style="font-size:14px;white-space:pre-wrap">{content}</pre><p>---</p><table><tr><td><b>发件人</b></td><td>{contacts}</td></tr><tr><td><b>号码</b></td><td>{phone}</td></tr><tr><td><b>时间</b></td><td>{datetime}</td></tr></table>"` | 短信消息模板 |
| `template_alert` | string | `"<b>短信即将存满！</b><br>已用: {total_count}/{max_count}"` | 告警消息模板 |
| `separator` | string | `"<hr>"` | 多条短信之间的分隔符 |

### 标准推送模式 (`"standard"`)

| 键 | 类型 | 默认值 | 说明 |
|-----|------|---------|------|
| `appToken` | string | `""` | 应用令牌（以 `AT_` 开头） |
| `uids` | string[] | `[]` | 目标用户 UID 列表（最多 2000） |
| `topicIds` | int[] | `[]` | 主题 ID 列表（最多 5） |

### 极简推送模式 (`"spt"`)

| 键 | 类型 | 默认值 | 说明 |
|-----|------|---------|------|
| `spt` | string | `""` | 单个 SPT 令牌（以 `SPT_` 开头） |
| `sptList` | string[] | `[]` | SPT 令牌列表（最多 10） |

## 模板变量

| 变量 | 适用范围 | 说明 |
|------|---------|------|
| `{phone}` | 短信 | 发件人号码 |
| `{contacts}` | 短信 | 联系人姓名 |
| `{content}` | 短信 | 短信正文 |
| `{timestamp}` | 短信 | Unix 时间戳 |
| `{datetime}` | 短信 | 可读时间（如 `2026-06-22 14:30:00`） |
| `{inbox_count}` | 通用 | 收件箱短信数量（通过 IPC 实时查询） |
| `{unread_count}` | 通用 | 未读短信数量 |
| `{outbox_count}` | 通用 | 发件箱短信数量 |
| `{draft_count}` | 通用 | 草稿箱短信数量 |
| `{total_count}` | 通用 | 所有箱体短信总数 |
| `{max_count}` | 通用 | 设备短信最大容量 |

当 `contentType=2` (HTML) 时，变量值会自动进行 HTML 转义；模板字符串本身可以包含 HTML 标签。

字面花括号使用 `{{` 表示 `{`，`}}` 表示 `}`。

## API 限制

- 频率限制：约 2 QPS（后端内置 500ms 最小间隔）
- 内容最大长度：40000 字符
- 消息保留：7 天

## 示例：标准推送

```json
{
  "wxpusher": {
    "mode": "standard",
    "appToken": "AT_abc123",
    "uids": ["UID_xxx", "UID_yyy"],
    "topicIds": [123],
    "contentType": 3,
    "template_msg": "# 来自 {contacts} 的短信\n{phone}\n---\n{content}"
  }
}
```

## 示例：极简推送（自用通知）

```json
{
  "wxpusher": {
    "mode": "spt",
    "spt": "SPT_abc123",
    "contentType": 2,
    "template_msg": "<b>新短信</b><br>{content}"
  }
}
```
