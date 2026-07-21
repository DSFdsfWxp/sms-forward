
# sms-forward

品速 r200 设备上的短信转发程序。读取未读短信，推送到微信、邮件等服务。

## 优势

- **轻量**: 不依赖 sqlite3，通过 IPC 直连设备数据库模块，程序体积小。
- **性能**: 事件驱动，不轮询，不过度占用单核 CPU。
- **插件式后端**: 内置 `wxpusher` 和 `smartroute`，新增后端只需实现 6 个函数。
- **热重载**: 配置支持热重载，切换后端或修改模板无需重启。

## 安装

事先说明：release 包仅在固件版本 `6.00.7` 和 `6.00.8` 测试过。

```bash
# 1. 将 release 包传到设备上，然后 SSH 登录设备
scp sms-forward-1.0.0.tar.gz root@192.168.1.1:/tmp/
ssh root@192.168.1.1
```

或者如果没有 ssh，用 adb 也是可以的。

```bash
# 1. 将 release 包传到设备上，然后 adb shell 登录设备
adb push sms-forward-1.0.0.tar.gz /tmp/
adb shell
```

完成后接下来步骤都一样。

```bash
# 2. 解压并安装
mkdir -p /tmp/sms-forward
tar xzf /tmp/sms-forward-1.1.0.tar.gz -C /tmp/sms-forward
/tmp/sms-forward/install.sh

# 3. 编辑配置文件
vi /home/root/sms/config.json

# 4. 热重载使配置生效
systemctl reload sms-forward
```

## 配置

配置文件 `/home/root/sms/config.json`，支持热重载。

键写法灵活，以下两种方式等价:

```json
{ "sms.autoClean": true, "sms.minInboxMsgNum": 50 }
```

```json
{ "sms": { "autoClean": true, "minInboxMsgNum": 50 } }
```

配置全貌示例（wxpusher）:

```json
{
  "push": {
    "backendName": "wxpusher",
    "minPushIntervalMs": 5000,
    "wxpusher": {
      "mode": "standard",
      "appToken": "AT_xxxxxxxxxxxxxxxxxxxx",
      "uids": ["UID_xxxxxxxxxxxxxxxxxxx"]
    }
  },
  "sms": {
    "autoClean": false,
    "msgCountThreshold": 75
  }
}
```

全局键:

| 键 | 默认值 | 说明 |
|---|---|---|
| `push.backendName` | `""` | 激活的后端名称 |
| `push.minPushIntervalMs` | `0` | 推送最小间隔(ms) |
| `sms.autoClean` | `false` | 短信满时自动清理 |
| `sms.allowCleanOutbox` | `false` | 允许清理发件箱 |
| `sms.minInboxMsgNum` | `75` | 收件箱保留最小条数 |
| `sms.msgCountThreshold` | `75` | 短信总量告警阈值 |

后端专属配置在 `push.<后端名>.*` 命名空间下，详见各后端文档。

## 卸载

```bash
# 1. SSH 登录设备
ssh root@192.168.1.1

# 2. 执行卸载脚本
/home/root/sms/uninstall.sh
```

## 开发

```bash
git clone <repo>
cd sms-forward
```

首次构建需从设备提取依赖库到 `third/lib/`（参考 [third/readme.md](third/readme.md)），然后:

```bash
xmake f
xmake
```

产物在 `build/linux/armv7/release/`。打包发行版:

```bash
./build-release.sh
```

产物在 `build/sms-forward-x.x.x.tar.gz`。

详细指南参见 [AGENTS.md](AGENTS.md)。

## 内置后端

内置多种推送后端，详情见 [后端列表](doc/backend/index.md)。
