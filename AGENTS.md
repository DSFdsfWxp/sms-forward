# sms-forward — Agent Guide

## Project

品速 r200 设备上的短信转发程序。读取设备 SIM 卡的未读短信，通过 IPC 与系统模块（SMS/DBM/MNET）通信，再通过可插拔的后端推送到外部服务。

- **语言**: C (gnu17), 交叉编译到 `armv7-eabihf`
- **构建**: `xmake` (跨平台构建系统), 自动下载 bootlin 工具链
- **设备固件版本**: 6.00.7, libcurl.so.4.5.0

## 构建 & 命令

```bash
xmake config             # 配置交叉编译 (armv7-eabihf)
xmake                    # 构建产物在 build/ 下
xmake f -c && xmake      # 清理重建

# 部署到设备后:
sms-forward-reload     # 热重载配置 (res/sms-forward-reload)
systemctl start sms-forward    # systemd 管理 (res/sms-forward.service)
```

项目无测试基础设施（嵌入式固件），无 linter/formatter 自动化（`.clang-format` 配置了 Google style + 2 空格缩进，手动使用）。

## 架构

源文件全部在 `src/` 下:

| 目录 | 职责 |
|---|---|
| `core/push.c` | 推送引擎: 专用线程 + 队列, 批量分发任务, 受 `push.minPushIntervalMs` 控制节流 |
| `core/sms.c` | 通过 IPC 读取/清理设备短信, 有新消息时调 `push_submit_msgs()` |
| `core/config.c` | 加载 `/home/root/sms/config.json`, 监听 SIGUSR1 热重载 |
| `push/` | **推送后端实现目录** — `pushs.c` 定义 `push_backends[]` 注册表 |
| `util/` | http (libcurl), json, queue, setting, encrypt, encode, log, time, string, vector 等工具模块 |
| `api/` | 设备 SDK 头文件 (IPC 通信、加密等, 外源不可修改) |
| `cloud/` | 云端侧 JS（smartroute 云函数） |

**初始化顺序** (`main.c:14`):
```
sms_init() → push_init() → config_init() → sms_check_new_msg()
```

配置加载依赖 `setting_open(CONFIG_FILE_PATH)` (路径硬编码为 `/home/root/sms/config.json`)，配置采用 JSON 格式。

## 推送后端开发

这是你最需要关注的部分。后端采用插件式架构:

```c
// src/push/pushs.h — 后端接口
typedef struct push_backend_t {
  const char* name;           // 配置项 push.backendName 匹配此值
  uint32_t min_interval_ms;   // 推送最小间隔
  void (*init)();             // 初始化
  void (*dispose)();          // 清理
  void (*load_config)();      // 从全局 setting 读取配置
  void (*add_task)(push_task_t* task);  // 添加任务 (需自行管理内存)
  void (*submit)();           // 最终提交推送
} push_backend_t;
```

**实现步骤**:

1. 在 `src/push/` 下新建单文件 `.c`，以对接后端名称命名（如 `mybackend.c`）。`push_backend_t` 结构体本体定义在此文件中（声明为 `const` 全局变量，文件作用域默认外部链接）。
2. 如需多文件实现，则开同名文件夹（如 `mybackend/`），内部分割。
3. 在 `src/push/pushs.c` 中通过 `extern` 前向声明后端变量，并注册到 `push_backends[]` 数组:
   ```c
   extern const push_backend_t my_backend;

   const push_backend_t* const push_backends[] = {
     &my_backend,
     NULL   // 哨兵结束
   };
   ```
4. 配置 `push.backendName` (从 JSON 配置动态选择后端)
5. `load_config` 中使用 `setting_get_str/setting_get_int/setting_get_bool` 读取配置
6. 批量读取配置数组用 `json_load_str_array()` / `json_load_int_array()`（`util/json.h`）
7. 模板格式化使用 `str_template()` + `str_template_resolver` 回调（`util/string.h`）；模板占位符如 `{phone}`、`{content}`，未知占位符保持原样。字面花括号用 `{{` 和 `}}` 转义。
8. 消息缓冲区使用 `vector_t`（`util/vector.h`），在 `add_task` 中累积，`submit` 中发送后 `vector_clear`
9. HTML 转义使用 `str_escape()`（`util/string.h`）
10. 发起 HTTP 请求使用 `util/http.h` (基于 libcurl 封装)
11. 需要实时短信统计数据时，通过 `push_type_alert_smsbox_almost_full` 任务的 `msg_count` 字段获取，该字段由引擎查询 DBM 后填充（`IPC_DBM_MSG_ID_GET_MSG_COUNT`）。后端无需自行发起 IPC。
12. 加密使用 `util/encrypt.h` (AES-CBC/ECB) 或 `api/encrypt.h` (设备硬件加密)
13. 若后端有配套的云端代码，在 `cloud/` 下放同名脚本或文件夹
14. 完成后端后，在 `doc/backend/名称.md` 编写文档，并更新 `doc/backend/index.md`

### 推送任务类型

```c
typedef struct push_task_t {
  enum {
    push_type_msgs,                  // 短信记录推送
    push_type_alert_smsbox_almost_full  // 短信存满告警
  } type;
  sms_record_t* records;            // NULL-terminated 数组 (push_type_msgs)
  dbm_get_msg_count_res_t msg_count; // DBM 统计数据 (push_type_alert)
} push_task_t;
```

`add_task` 中收到的 `push_task_t` 由引擎管理内存，如需异步处理应在内部深拷贝（`sms_records_free` 由 `pushs_dispose_task` 调用）。告警任务的 `msg_count` 字段由引擎通过 IPC 查询 DBM 后填充，后端直接使用。

### 后端节流

引擎每次 `submit()` 后至少等待 `max(push.minPushIntervalMs, backend->min_interval_ms)` 毫秒。若后端 API 有频率限制，在 `min_interval_ms` 中声明。

## 设备 IPC 通信

通过 `frwk_ipc_send_sync()` 与其他系统模块通信，消息 ID 和结构体定义见 `api/ipc/*.h`。调用方式在 `src/core/sms.c` 中有完整示例。

**限制**: 不要轻易使用 `frwk_ipc_send_sync`。IPC 的请求/响应结构体、消息 ID 和字段含义取决于设备 SDK，除非能从 `api/ipc/*.h` 明确推断出参数，或用户明确告知了 IPC 接口信息，否则不要调用。

## 工具模块

| 模块 | 职责 |
|------|--------|
| **vector** | `util/vector.h` — 泛型动态数组，`elem_size` 初始化时指定；每次 push_n 后自动零填充（char 向量即 C 字符串） |
| **string** | `util/string.h` — HTML 转义 (`str_escape`) + `{var}` 模板引擎 (`str_template` + resolver 回调) |
| **json** | `util/json.h` — 类型检查、点号路径导航、从 JSON 数组提取 C 数组 |
| **http** | `util/http.h` — 基于 libcurl 的 HTTP 请求封装 |
| **setting** | `util/setting.h` — JSON 配置读取 |
| **queue** | `util/queue.h` — 定长元素、动态增长、线程安全队列 |
| **encode** | `util/encode.h` — hex 编解码 (`encode_hex_decode`) |
| **encrypt** | `util/encrypt.h` — AES-CBC/ECB 加密 + Base64 输出 |
| **log** | `util/log.h` — 日志宏 `LOG_I/W/E/D`，使用前需 `#define LOG_TAG "模块名"` |
| **time** | `util/time.h` — 单调时钟 + 毫秒睡眠 |

### memory 约定

- `setting_get_str()` 返回 `strdup()` 的字符串 → 用 `free()`
- `json_print_buffered()` 返回设备分配的字符串 → 用 `os_mem_free()`
- `str_template()` / `str_escape()` 返回 `malloc()` 的字符串 → 用 `free()`
- `json_load_str_array()` 返回的数组和内部字符串 → 逐一 `free()` 后 `free()` 数组
- `json_load_int_array()` 返回的数组 → `free()`
- JSON 节点树 → `json_free()`
- `vector_free()` 释放内部存储，不释放 `vector_t` 结构体本身
- 后端 `dispose()` / `load_config()` 必须确保释放所有动态分配的状态

### 工具模块编写规范

- 工具函数若有泛用性（不绑定特定后端逻辑），优先提取为 `util/` 下的独立模块
- 头文件 `src/util/xxx.h` + 实现 `src/util/xxx.c`，xmake 通配符自动编译
- 使用 `#pragma once`，依赖仅限 `api/` 和同级 `util/` 模块，禁止反向依赖 `core/`、`push/`
- 所有导出函数写 Doxygen 注释：`@brief` / `@param` / `@return`，注明内存所有权（谁分配、谁释放）
- API 保持最小聚焦，一个模块一个职责

## 配置模块

配置统一由 `util/setting.h` 管理，核心流程：

```c
setting_open(path)   // 解析 JSON 文件，阻塞 SIGUSR1
setting_reload()     // 原子重载，失败时回退旧配置
```

- 配置文件路径硬编码为 `/home/root/sms/config.json`
- 支持 SIGUSR1 热重载（`res/sms-forward-reload`）
- `core/config.c` 协调各模块加载: `config_load()` → `sms_load_config()` + `push_load_config()`
- `push_load_config()` 按 `push.backendName` 动态选择后端

### setting 读取函数

| 函数 | 返回值 | 释放方式 |
|------|--------|---------|
| `setting_get_str(key, def)` | `char*` (strdup) | `free()` |
| `setting_get_int(key, def)` | `uint64_t` | - |
| `setting_get_float(key, def)` | `double` | - |
| `setting_get_bool(key, def)` | `bool` | - |
| `setting_get_raw(key)` | `const json_t*` | 不释放（内部持有） |

`key` 支持点号路径访问嵌套对象，例如 `"push.backendName"` 对应 `{"push": {"backendName": "mybackend"}}`。

### 后端专属配置

各后端在 `push.<backendName>.*` 命名空间下定义自己的配置，详见 `doc/backend/` 下对应文档。

### 全局配置键

| 键 | 默认值 | 说明 |
|---|---|---|
| `push.backendName` | `""` | 激活的后端名称 |
| `push.minPushIntervalMs` | `0` | 推送最小间隔(ms) |
| `sms.autoClean` | `false` | 短信满时自动清理 |
| `sms.allowCleanOutbox` | `false` | 允许清理发件箱 |
| `sms.minInboxMsgNum` | `75` | 收件箱保留最小条数 |
| `sms.msgCountThreshold` | `75` | 短信总量告警阈值 |

### 后端处理流程模式

```
add_task(task) → 记录任务到内部缓冲区
submit()       → 将缓冲区中所有任务批量发送 → 清空缓冲区
```

## 代码风格

- 2 空格缩进, 80 列宽, Google 风格 (`.clang-format` 已配置)
- 指针/引用左对齐 (`int* p`)
- `if`/`while` 等控制语句必须使用花括号（clang-format 配置 `AllowShortIfStatementsOnASingleLine: Never`)
- Include 顺序: 标准库 → SDK API → 项目模块 → 对应头文件，组间空行分隔
- 日志宏: `LOG_I` `LOG_W` `LOG_E` `LOG_D`，需先定义 `LOG_TAG`
- 文档文件 (`*.md`) 统一使用中文
- VSCode 使用 clangd，设置 `--header-insertion=never` (避免自动插入头文件)
