
# SmartRoute 后端

将短信记录序列化为 JSON 数组，AES-256-CBC 加密后发送至阿里云函数。云函数解密后对内容进行多语言敏感信息检测，匹配则路由到 SMTP 邮件发送，否则转发至 WxPusher。

## 设备端配置 (`push.smartroute.*`)

| 键 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `apiUrl` | string | `""` | 阿里云函数 HTTP 触发器 URL |
| `apiToken` | string | `""` | 鉴权令牌（作为 Authorization header 发送） |
| `aesKey` | string | `""` | AES-256 密钥，64 字符 hex |
| `aesIv` | string | `""` | AES IV，32 字符 hex |

```json
{
  "push": {
    "backendName": "smartroute",
    "smartroute": {
      "apiUrl": "https://xxx.cn-hangzhou.fc.aliyuncs.com/...",
      "apiToken": "Bearer my-secret-token",
      "aesKey": "603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4",
      "aesIv": "000102030405060708090a0b0c0d0e0f"
    }
  }
}
```

## 云端环境变量

### 加密 / SMTP

| 变量 | 说明 |
|------|------|
| `TOKEN` | 请求鉴权令牌（为空则不校验），设备端 `apiToken` 需与此一致 |
| `AES_KEY` | 与设备端一致的 AES-256 密钥（64 hex） |
| `AES_IV` | 与设备端一致的 IV（32 hex） |
| `SMTP_HOST` | SMTP 服务器地址 |
| `SMTP_PORT` | SMTP 端口（465/587） |
| `SMTP_USER` | SMTP 用户名 |
| `SMTP_PASS` | SMTP 密码 |
| `SMTP_TO` | 邮件送达地址 |

### WxPusher 推送（与本地 wxpusher 后端同等能力）

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `WXPUSHER_MODE` | `"standard"` | `"standard"` 或 `"spt"` |
| `WXPUSHER_APP_TOKEN` | `""` | 标准模式应用令牌 |
| `WXPUSHER_UIDS` | `""` | 目标 UID（逗号分隔） |
| `WXPUSHER_TOPIC_IDS` | `""` | 主题 ID（逗号分隔） |
| `WXPUSHER_SPT` | `""` | SPT 模式令牌 |
| `WXPUSHER_SPT_LIST` | `""` | SPT 列表（逗号分隔） |
| `CONTENT_TYPE` | `2` | 1=text, 2=html, 3=markdown |
| `TEMPLATE_EMAIL` | `"From: {contacts} ({phone})\nTime: {datetime}\n\n{content}"` | 邮件模板（文本），自动转换为 HTML 版本 |
| `TEMPLATE_EMAIL_ALERT` | `"SMS storage almost full!\nUsed: {total_count} / {max_count}"` | 告警邮件模板 |
| `TEMPLATE_MSG` | `"<b>新短信</b><br>{phone} ({contacts})<br><pre>{content}</pre>"` | 短信消息模板 |
| `TEMPLATE_ALERT` | `"<b>短信即将存满！</b><br>已用: {total_count}/{max_count}"` | 告警消息模板 |
| `SUMMARY` | `""` | 推送摘要 |
| `URL` | `""` | 消息链接 |

模板变量：`{phone}` `{contacts}` `{content}` `{timestamp}` `{datetime}` `{total_count}` `{max_count}` `{inbox_count}` `{unread_count}` `{outbox_count}` `{draft_count}`。

字面花括号用 `{{` 和 `}}` 转义。

## 内容检测规则

关键词匹配采用多语言正则 alternation，验证码/取件码类仅在关键词前后 10+30 字符窗口内检查数字/字母，避免全局误匹配。

### phone

| 格式 | 匹配 |
|------|------|
| 明文 | `1[3-9]\d{9}` |
| 遮盖 | `1[3-9]\d{4}****\d{4}` |
| 国际前缀 | 可选 `+86` / `0086` |

### code (验证码)

中/英/日/韩 20+ 关键词：`验证码` `安全码` `激活码` `verification code` `otp` `mfa` `passcode` `認証コード` `ワンタイムパスワード` `인증번호` `인증코드` 等，匹配后检查附近 4-8 位数字。

### pickup (取件码)

中/英/日/韩 14+ 关键词：`取件码` `取货码` `取餐码` `pickup code` `parcel` `tracking code` `受け取りコード` `픽업코드` 等，匹配后检查附近 4-12 位字母数字。

### password (密码)

中/英/日/韩 20+ 关键词：`密码` `登录密码` `支付密码` `pin` `PIN` `password` `passcode` `access code` `暗証番号` `pinコード` `비밀번호` `pin번호` 等，关键词存在即匹配。

## 请求协议

设备端 POST 纯文本 Body（Base64 编码的 AES 密文），无额外 JSON 包裹。云函数直接取 `event.body` 解密。

鉴权：若配置了 `TOKEN` 环境变量，云函数校验请求头 `Authorization: Bearer <token>` 或 `Authorization: <token>`，不匹配返回 401。

## 数据格式

### 短信记录

```json
[
  {
    "phone": "13800138000",
    "contacts": "张三",
    "content": "【淘宝】您的验证码是123456",
    "timestamp": 1687392000
  }
]
```

### 告警记录

```json
{
  "type": "alert",
  "content": "SMS storage almost full",
  "total_count": 75,
  "max_count": 100,
  "inbox_count": 60
}
```
