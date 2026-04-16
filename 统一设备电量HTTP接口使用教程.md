# EasyBluetooth工具：https://softnav.top/bluetooth/

# 统一设备电量 HTTP 接口（VIP）使用教程
欢迎基于本接口开发皮肤、插件及扩展应用。
商业集成请联系作者授权。
## 1. 功能说明
统一设备电量 HTTP 接口用于将 EasyBluetooth 内部设备状态以 JSON 形式输出给第三方软件或者硬件（如 Rainmeter、Wallpaper Engine、stream deck）。

- 功能权限：仅 VIP 可用
- 数据来源：应用内存快照（不会在每次请求时重新扫描蓝牙）
- 默认监听：本机 `127.0.0.1`
## 2. 在设置中开启
打开：`设置 -> 高级功能（PRO） -> 统一标准数据api设置（按钮）`

点击该按钮后会弹出独立窗口，在窗口中完成统一接口的开关与参数设置。

推荐配置：
1. 开启 `开启统一标准数据接口`
2. 端口保持默认 `18080`
3. 监听范围选择 `仅本机（localhost）`
4. 若担心同机进程读取，开启 `Token 鉴权` 并设置一个随机 Token

## 3. 接口地址
- 状态接口：`GET /api/v1/status`
- 本机完整地址示例：`http://127.0.0.1:18080/api/v1/status`

## 4. 返回格式（统一响应）
所有响应都使用统一结构：

```json
{
  "code": 200,
  "message": "操作成功",
  "data": {
    "schemaVersion": 1,
    "generatedAtUtc": "2026-02-13T10:00:00+00:00",
    "appVersion": "3.0.1",
    "devices": []
  }
}
```

### `devices` 字段说明
- `id`：脱敏设备标识（哈希短串，不暴露原始 MAC/devicePath）
- `name`：设备原始名称（来自系统/协议识别，不受用户重命名影响）
- `renamedName`：用户重命名后的名称，未重命名时为 `null`
- `deviceType`：设备类别，优先取用户选择的预设图标类别；否则按设备原始名称自动识别，固定为 `general` / `headphones` / `mouse` / `keyboard` / `speaker` / `gamepad`
- `source`：数据来源（如 `classic`、`ble`、`airpods`、`logitech`）
- `status`：`online` / `offline` / `unknown`
- `connectionStatus`：内部连接状态文本
- `battery`：主电量（0~100）；未知且此前没有有效电量时为 `null`。设备休眠时若存在最近一次有效电量，则保留该值
- `isCharging`：是否充电中
- `isSleeping`：是否休眠；为 `true` 时不代表 `battery` 一定为 `null`
- `isBatteryUnsupported`：是否设备不支持标准电量读取
- `isShownInTray`：该设备当前是否被用户指定为“显示在托盘”
- `batteryLastUpdatedUtc`：电量上次更新时间（UTC，ISO 8601），与主窗口“更新于”语义一致；休眠更新不会刷新该时间，但会保留已有值
- `airPodsLeftBattery` / `airPodsRightBattery` / `airPodsCaseBattery`：AirPods 细分电量（未知为 `null`）

可见性规则：
- `devices` 仅返回主窗口可见设备，用户主动隐藏的设备不会出现在接口结果中。

### 4.1 单个设备对象完整结构（详细版）

```json
{
  "id": "a1b2c3d4e5f6",
  "name": "My AirPods Pro",
  "renamedName": "通勤耳机",
  "deviceType": "headphones",
  "source": "airpods",
  "status": "online",
  "connectionStatus": "CONNECTED",
  "battery": 78,
  "isCharging": false,
  "isSleeping": false,
  "isBatteryUnsupported": false,
  "isShownInTray": true,
  "batteryLastUpdatedUtc": "2026-02-13T09:58:25+00:00",
  "airPodsLeftBattery": 80,
  "airPodsRightBattery": 78,
  "airPodsCaseBattery": 62
}
```

### 4.1.1 休眠设备示例（保留最近一次有效电量）

```json
{
  "id": "b7c8d9e0f1a2",
  "name": "Razer Viper V3 Pro",
  "renamedName": null,
  "deviceType": "mouse",
  "source": "razer",
  "status": "online",
  "connectionStatus": "CONNECTED",
  "battery": 24,
  "isCharging": false,
  "isSleeping": true,
  "isBatteryUnsupported": false,
  "isShownInTray": false,
  "batteryLastUpdatedUtc": "2026-03-11T02:15:43+00:00",
  "airPodsLeftBattery": null,
  "airPodsRightBattery": null,
  "airPodsCaseBattery": null
}
```

### 4.2 字段详细定义（建议第三方按此实现）

| 字段 | 类型 | 取值/范围 | 是否必有 | 说明 |
|------|------|-----------|----------|------|
| id | String | 12位十六进制短串 | 是 | 设备脱敏ID，仅用于本地匹配 |
| name | String | 任意字符串 | 是 | 设备原始名称，不受用户重命名影响 |
| renamedName | String/Null | 任意字符串或 null | 是 | 用户重命名后的显示名称；未重命名时为 null |
| deviceType | String | general/headphones/mouse/keyboard/speaker/gamepad | 是 | 设备类别；优先取用户选择的预设图标类别，否则按设备原始名称自动识别 |
| source | String | classic/ble/airpods/xbox/playstation/logitech/razer/windowsbattery/rog/atk/hyperx/rapoo/mchose/valkyrie/unknown | 是 | 数据来源 |
| status | String | online/offline/unknown | 是 | 统一连接状态语义 |
| connectionStatus | String | 例如 CONNECTED | 是 | 原始连接状态文本（用于调试） |
| battery | Integer/Null | 0~100 或 null | 是 | 主电量；未知且此前没有有效电量时为 null。休眠时若已有最近一次有效电量，则继续返回该值 |
| isCharging | Boolean | true/false | 是 | 是否充电 |
| isSleeping | Boolean | true/false | 是 | 是否休眠；为 true 时不代表 battery 必为 null |
| isBatteryUnsupported | Boolean | true/false | 是 | 是否设备不支持标准电量读取 |
| isShownInTray | Boolean | true/false | 是 | 当前该设备是否被用户指定为“显示在托盘”；未指定时为 false |
| batteryLastUpdatedUtc | String/Null | ISO 8601 UTC 或 null | 是 | 电量上次更新时间；语义与主窗口“更新于”一致。休眠更新不刷新该时间，但会保留已有值 |
| airPodsLeftBattery | Integer/Null | 0~100 或 null | 是 | AirPods 左耳电量，非AirPods通常为null |
| airPodsRightBattery | Integer/Null | 0~100 或 null | 是 | AirPods 右耳电量，非AirPods通常为null |
| airPodsCaseBattery | Integer/Null | 0~100 或 null | 是 | AirPods 盒电量，非AirPods通常为null |

### 4.3 建议的健壮性处理

- 解析时允许未知新增字段（向前兼容）
- 对 `battery: null` 做明确分支，不要按 `0` 处理
- 对 `source` 与 `status` 保留默认分支（unknown）
- 对 `deviceType` 保留默认分支，建议第三方将其作为图标映射字段使用
- AirPods 细分字段可能为 `null`，UI 应允许隐藏
- 如需显示用户看到的设备名称，建议优先使用 `renamedName ?? name`

## 5. 错误码
- `401`：鉴权失败（启用了 Token 但未提供或不匹配）
- `403`：非 VIP 用户调用
- `404`：路径不存在
- `405`：请求方法错误（仅支持 GET）
- `500`：内部错误
- `503`：鉴权开关已开但 Token 未配置

## 6. schemaVersion 说明
`schemaVersion` 是“接口结构版本号”，不是应用版本号。

- 当前固定：`1`
- 兼容新增字段：`schemaVersion` 保持不变
- 本次新增 `deviceType` 属于兼容新增字段，不提升 `schemaVersion`
- 只有“破坏性变更”（删字段、改字段类型、改语义）才会升级到 `2`

## 7. appVersion 的作用

`appVersion` 是应用版本号（例如 `3.0.1`），主要用于：

- 第三方皮肤做兼容分支（旧版本兼容逻辑）
- 用户反馈问题时快速定位版本
- 日志排障时关联接口输出与程序版本

是否必须：

- 对“只显示电量”的最小场景，不是必须
- 对“长期维护兼容性”的场景，建议保留

当前建议：保留 `appVersion`，因为它不属于敏感信息，但能显著提升排障效率。

## 8. 使用建议
- 如果你使用 Token：在请求头添加 `X-Api-Token`
- 若有多设备，建议优先按 `id` 做匹配；`name` / `renamedName` 更适合作为展示字段
- 若开启了 LAN 模式，请注意 Windows URLACL/防火墙策略

## 9. 用户可见性与隐私说明

这份教程可以直接发给终端用户查看。当前接口设计默认不暴露高敏感信息：

- 不返回完整 MAC 地址
- 不返回完整 devicePath
- `id` 是不可逆脱敏短标识

需要注意的边界：

- `name` 为设备原始名称；`renamedName` 可能包含用户自定义名称（例如真实姓名/昵称），若用于公开截图建议打码
- 开启 LAN 监听时，建议同时开启 Token 鉴权
- Token 属于敏感信息，不应截图或分享给他人

## 10. LAN 监听报“拒绝访问”（URLACL）如何处理

当你在 LAN 模式下看到类似日志：

- `启动失败: 拒绝访问。`
- `System.Net.HttpListenerException`

通常表示 Windows 的 URLACL 权限未授予当前用户，导致 `http://+:端口/` 无法监听。

### 10.1 在应用内一键修复（推荐）

进入 `设置 -> 高级功能（PRO） -> 统一标准数据api设置`，点击按钮：

- `一键修复网络连通性`

点击后应用会请求管理员权限并自动执行修复命令。修复完成后，重新启用（或切换一次）LAN 监听即可。

### 10.2 手动修复命令（管理员 PowerShell / CMD）

请将 `18080` 替换为你实际使用的端口（例如 `18081`）：

```bat
netsh http delete urlacl url=http://+:18080/
netsh http delete urlacl url=http://127.0.0.1:18080/
netsh http delete urlacl url=http://localhost:18080/

netsh http add urlacl url=http://+:18080/ user=Everyone
netsh http add urlacl url=http://127.0.0.1:18080/ user=Everyone
netsh http add urlacl url=http://localhost:18080/ user=Everyone

netsh advfirewall firewall add rule name="EasyBluetooth LAN 18080" dir=in action=allow protocol=TCP localport=18080 profile=any
```

例如当前登录用户是 `MYPC\Alice`，端口是 `18081`：

```bat
netsh http delete urlacl url=http://+:18081/
netsh http delete urlacl url=http://127.0.0.1:18081/
netsh http delete urlacl url=http://localhost:18081/

netsh http add urlacl url=http://+:18081/ user=Everyone
netsh http add urlacl url=http://127.0.0.1:18081/ user=Everyone
netsh http add urlacl url=http://localhost:18081/ user=Everyone

netsh advfirewall firewall add rule name="EasyBluetooth LAN 18081" dir=in action=allow protocol=TCP localport=18081 profile=any
```

## 语义更新（2026-03）

- `battery = null` 表示当前没有有效电量值，且当前设备此前也没有可保留的有效电量；可能由暂时读取失败、首次发现即休眠/待机无响应，或设备本身不支持该读取路径导致。
- `isSleeping = true` 时，若设备此前存在最近一次有效电量，则 `battery` 与 `batteryLastUpdatedUtc` 会继续返回该值；若此前没有有效电量，才会保持 `null`。
- `isBatteryUnsupported = false` 且 `battery = null` 时，表示设备仍在线，但当前没有可确认的有效电量；客户端展示建议统一渲染为 `--`。
- `isBatteryUnsupported = true` 且 `battery = null` 时，表示设备存在，但当前路径不支持读取；客户端如果只关心展示，也建议统一渲染为 `--`。
- `batteryLastUpdatedUtc = null` 不代表设备离线，只表示当前没有可确认的有效电量时间戳可供保留。
- 自 2026-03 起，统一接口保留 `isSleeping` 作为技术状态位；休眠设备如果已有最近一次有效电量，会继续返回该电量和对应时间，`status` 仍按在线/离线语义处理。
- `name` 固定表示设备原始名称；`renamedName` 用于输出用户重命名后的名称，未重命名时为 `null`。
- `deviceType` 表示设备类别而不是图标来源类型；当用户选择了预设图标时优先输出该图标类别，否则按设备原始名称自动识别。
- `isShownInTray = true` 仅表示该设备当前被用户手动指定为“显示在托盘”；如果托盘仍走默认最低电量逻辑，则所有设备都为 `false`。

如仍无法被局域网其他设备访问，再检查防火墙入站放行：

```bat
netsh advfirewall firewall add rule name="EasyBluetooth LAN 18081" dir=in action=allow protocol=TCP localport=18081 profile=any
```
