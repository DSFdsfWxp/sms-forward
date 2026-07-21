# SMTP 邮件后端

通过 SMTP 协议将短信推送为邮件。基于 GnuTLS 直接实现 SMTP over TLS (SMTPS)，兼容大多数邮件服务商（Gmail、QQ 邮箱、163 邮箱、企业邮箱等）。

## 特性

- **原始 socket + GnuTLS**：不依赖 libcurl 的 SMTP 模块，更轻量可控
- **TLS 会话复用**：首次完整握手后，后续连接复用 TLS 会话（跳过证书校验与密钥交换），大幅降低延迟
- **AUTH LOGIN 认证**：Base64 编码用户名/密码
- **隐式 TLS (SMTPS)**：直连 TLS，默认端口 465

## 配置

```json
{
  "push": {
    "backendName": "smtp",
    "smtp": {
      "server": "smtp.example.com",
      "port": 465,
      "username": "user@example.com",
      "password": "your-password",
      "from": "sms-alert@example.com",
      "to": ["admin@example.com"],
      "contentType": 2,
      "subject": "[SMS] {contacts}",
      "minIntervalMs": 5000,
      "timeout": 30
    }
  }
}
```

### 配置项

| 键 | 类型 | 默认值 | 说明 |
|----|------|--------|------|
| `server` | string | `""` | SMTP 服务器地址 |
| `port` | int | `465` | SMTP 端口（隐式 TLS / SMTPS） |
| `username` | string | `""` | AUTH LOGIN 用户名，空则跳过认证 |
| `password` | string | `""` | AUTH LOGIN 密码 |
| `from` | string | `""` | 发件人邮箱 |
| `contentType` | int | `2` | 1=纯文本, 2=HTML |
| `to` | string[] | `[]` | 收件人邮箱列表 |
| `subject` | string | `"[SMS] {contacts}"` | 邮件主题模板（始终纯文本） |
| `template_msg` | string | 见下方 | 短信消息模板 |
| `template_alert` | string | 见下方 | 告警消息模板 |
| `separator` | string | `"<hr>"` | 多条短信之间的分隔符 |
| `minIntervalMs` | int | `5000` | 最小发件间隔（毫秒） |
| `timeout` | int | `30` | SMTP 连接超时（秒） |

> **注意**：`useTls` 配置项已移除。当前实现始终使用隐式 TLS (SMTPS)，直连服务器即进行 TLS 握手。

### contentType

| 值 | Content-Type | 变量转义 |
|----|-------------|---------|
| `1` | `text/plain` | 不转义，直接替换 |
| `2` | `text/html`（**默认**） | `{phone}` `{contacts}` `{content}` 自动 HTML 转义 |

### 默认模板

**短信模板 (`template_msg`)：**
```html
<h2>新短信</h2>
<pre style="font-size:14px;white-space:pre-wrap">{content}</pre>
<hr><table>
<tr><td><b>发件人</b></td><td>{contacts}</td></tr>
<tr><td><b>号码</b></td><td>{phone}</td></tr>
<tr><td><b>时间</b></td><td>{datetime}</td></tr>
</table>
```

**告警模板 (`template_alert`)：**
```html
<h2>短信即将存满</h2>
<p>已用: <b>{total_count}</b> / <b>{max_count}</b></p>
```

## 模板变量

`{phone}` `{contacts}` `{content}` `{timestamp}` `{datetime}`
`{inbox_count}` `{unread_count}` `{outbox_count}` `{draft_count}` `{total_count}` `{max_count}`

## 纯文本模式示例

设置 `contentType: 1` 后需同步修改模板为纯文本：

```json
{
  "smtp": {
    "contentType": 1,
    "template_msg": "{content}\n=====\n来自: {contacts} ({phone})\n时间: {datetime}",
    "template_alert": "短信即将存满!\n已用: {total_count} / {max_count}",
    "separator": "\n=====\n"
  }
}
```

## TLS 会话复用

本后端启动时初始化一次 GnuTLS 全局凭证和 X.509 CA 信任链。首次与 SMTP 服务器建立连接时执行完整 TLS 握手（证书校验 + 密钥交换）；后续连接自动复用缓存的 TLS 会话数据，跳过证书校验与密钥交换，显著降低每次发信的延迟。

会话缓存在内存中，进程重启后重新握手。

## 发件限制

`minIntervalMs` 可通过配置文件调整，默认 500ms。

## 示例：163 邮箱

```json
{
  "push": {
    "backendName": "smtp",
    "smtp": {
      "server": "smtp.163.com",
      "port": 465,
      "username": "your-email@163.com",
      "password": "your-auth-code",
      "from": "your-email@163.com",
      "to": ["receiver@example.com"],
      "minIntervalMs": 6000
    }
  }
}
```

> 163 邮箱需在设置中开启 SMTP 服务，使用生成的授权码作为密码。

## 示例：Gmail

```json
{
  "push": {
    "backendName": "smtp",
    "smtp": {
      "server": "smtp.gmail.com",
      "port": 465,
      "username": "your-account@gmail.com",
      "password": "your-app-password",
      "from": "your-account@gmail.com",
      "to": ["receiver@example.com"],
      "minIntervalMs": 10000
    }
  }
}
```

> 注意：Gmail 需使用应用专用密码（App Password），不支持直接使用账户密码。

## 示例：QQ 邮箱

```json
{
  "push": {
    "backendName": "smtp",
    "smtp": {
      "server": "smtp.qq.com",
      "port": 465,
      "username": "your-qq@qq.com",
      "password": "your-auth-code",
      "from": "your-qq@qq.com",
      "to": ["receiver@example.com"],
      "minIntervalMs": 12000
    }
  }
}
```

> QQ 邮箱需在设置中开启 SMTP 服务，使用生成的授权码作为密码。
