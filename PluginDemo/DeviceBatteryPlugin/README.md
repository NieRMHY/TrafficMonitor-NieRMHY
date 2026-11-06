# 多设备电量插件

保留原 HTTP API 单设备用法，并增加 FerrisPulse 本机快照和最多 8 个独立显示项目。未配置 `snapshot_file` 时继续使用旧 HTTP 设置，默认只有一个项目。

## FerrisPulse 配置

FerrisPulse 需要包含 `BatterySnapshotExporter`，并在它实际运行的 Windows 用户上下文的 `HKCU\Software\FerrisPulse` 设置 DWORD `ExportBatteryStatus=1`（新键缺失时兼容旧 `RazerBatteryTray` 键）。应用每 3 秒生成 `%LOCALAPPDATA%\FerrisPulse\battery-status.json` 心跳，硬件查询完成后立即更新其中的观察结果；首次查询前保持未知，不额外读取 HID。

在 TrafficMonitor 的配置目录 `plugins\DeviceBatteryPlugin.dll.ini` 写入以下内容。**包含中文时请保存为 UTF-16 LE BOM**，与 Windows Profile API 一致：

```ini
[api]
snapshot_file=%LOCALAPPDATA%\FerrisPulse\battery-status.json
refresh_interval_ms=3000
max_snapshot_age_seconds=900
max_heartbeat_age_seconds=15
[display]
item_count=4
trailing_spacer=0
show_charging_marker=0
align_columns=1
[device0]
source=Razer
label_text=雷蛇
[device1]
source=Dareu
label_text=AE7
[device2]
source=Maono
label_text=MIC
[device3]
source=Flydigi
label_text=手柄
```

重启 TrafficMonitor，在显示项目中启用四个电量项。项目 ID 依次为 `Device24GBattery`、`Device24GBattery_2`、`Device24GBattery_3`、`Device24GBattery_4`，原项目 ID 保留。

上述四设备方案包含八爪鱼 6；将 `Device24GBattery_4` 紧接 MIC 排列。它直接占用原来的空白下行，后面的硬件项目索引不变；任务栏显示为两列“雷蛇 / AE7”和“MIC / 手柄”。手柄配色为淡青绿 `#80DED3`。仅需三台时设 `item_count=3`，需要 MIC 下方留空再设 `trailing_spacer=1`；配置缺省值仍保持旧行为。

设置 `align_columns=1` 后，四项电量按当前实际字体测量统一的标签列和数值列，不再依赖细空格对齐。数值列测量 0%–100% 的最大宽度；即使使用比例字体，100% 也不会改变列宽。绘制会限制在自身矩形内，并保留列间距。这时标签从本插件的 `label_text` 设置读取，主程序的显示文本空格补齐不参与绘制；默认 0 保留原主程序绘制方式。

双行任务栏中，当 MIC 是最后一个单独项目时，TrafficMonitor 会把标签和数值拆成上下两行。可选 `trailing_spacer=1` 会增加空白项目 `Device24GBatterySpacer`，在显示项目中启用并紧接 MIC 排列，便能让 MIC 的标签/数值显示在同一行，下一行保持空白。空白项目不请求设备数据，也不占文本宽度；默认关闭以兼容旧配置。

紧凑布局从左至右，每列上下配对：

| 上行 | 下行 |
| --- | --- |
| 上传 | 下载 |
| 内存百分比 | 已用内存数值 |
| CPU 使用率 | CPU 频率（无前缀） |
| CPU 功耗（无前缀） | CPU 温度（无前缀） |
| 核显使用率 | 核显功耗 |
| 独显使用率 | 显存使用率 |
| 独显功耗（无前缀） | 独显温度（无前缀） |
| 固态1温度 | 固态2温度 |
| 雷蛇电量 | AE7 电量 |
| MIC 电量 | 手柄电量（未配置时保留空白） |

只在“雷蛇/AE7”标签前留一处设备组间距，MIC 与两台鼠标采用正常列间距。CPU 与独显的功耗、温度标签在 `config.ini` 的 `[plugin_display_str_taskbar_window]` 对应项目设为空字符串。新增占位项目后，手工保存的全局索引与配色需将后续 HardwareMonitor 项目后移一位。

整组右移只使用用户配置 `[task_bar] window_offset_left`，不修改定位代码。摄氏度保持与数值相同字号，HardwareMonitor.ini 的温度项目使用 `unit = " °C"`（单位前为 U+2009 细空格）略微拉开间距。CPU 使用率用整数百分比，项目间距设为 2；雷蛇/AE7 标签按本机字体宽度对齐。

副屏任务栏默认预留 88 像素时钟区域，正的 `window_offset_left` 会吃掉这段空间。本机现在使用 16 像素偏移，右侧仍保留约 70 像素；不要为了靠右而把偏移调大到覆盖时钟。

`show_charging_marker=0` 会省去电量数字后的 `+` 及其预留宽度，完整保留 0%–100% 和 `--`；充电信息仍在提示中显示。默认 1 保持旧的 `77%+` 行为。

NieRMHY 的任务栏配置将这四个项目放在固态温度后面，并使用固定设备配色：

| 项目 | 标签 | RGB 颜色 |
| --- | --- | --- |
| Device24GBattery | 雷蛇 | 深绿 `#3CC878` |
| Device24GBattery_2 | AE7 | 粉色 `#F49AC2` |
| Device24GBattery_3 | MIC | 橙色 `#FFB454` |
| Device24GBattery_4 | 手柄 | 淡青绿 `#80DED3` |

这些颜色由 TrafficMonitor 的每项目颜色设置控制。FerrisPulse 自身使用按剩余电量变化的绿/黄/红规则，两边共享数值，显示配色分别设置。

选择优先级为 `device_id`、`device_name`、`source`，显式选择不存在时显示 `--`，不会退回另一台设备。多个同品牌设备请用快照内 `id` 配置 `device_id`。未指定选择条件时保留旧的托盘设备优先策略。

`77%+` 是确认充电。设备休眠/离线、快照过期、生产进程退出、数据未知或文件不可读时显示 `--`，不把历史电量显示成当前数据。文件最大 256 KiB；生产 PID 必须存活且进程创建时间不得晚于文件时间。插件每 3 秒重新尝试，不缓存失败的文件句柄或旧生产 PID，因此任意启动顺序和发布者重启都可恢复。

新版快照的 `generatedAtUtc` 随心跳更新，`observedAtUtc` 只随硬件查询更新。默认心跳 15 秒失效（可设 5–300 秒），实际观察 900 秒失效（可设 30–3600 秒）。持续心跳不能掩盖卡住的硬件查询。无心跳字段的旧版快照仍按原文件时效检查；读取不会触碰 HID。

## 排查运行实例与独立测试不一致

`--` 的提示包含文件打开失败、心跳或观察过期、发布进程无法访问等具体原因。临时设置 `[api] diagnostics=1`，下次读取时即可启用，无需重启 TrafficMonitor。插件 INI 旁生成 `.0.poll.txt` / `.0.render.txt` 等诊断文件，分别记录实际读取和界面取值，包含宿主 PID、实际配置/快照路径与结果，不记录 API token。

应以正常从桌面启动的发布者生成的快照和任务栏宿主 PID 的 `render` 记录或屏幕结果确认恢复；调试终端的注册表开关和独立测试不能代替实际应用验证。完成排查后设 `diagnostics=0`，下次读取时停止写日志，无需重启。缺失文件恢复、后续数值变化、发布者重启、心跳恢复及过期观察均有不重载配置的回归测试。

## 构建和测试

使用 MSVC x64 工具链和 Windows SDK，在仓库根目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_device_battery.ps1 -VCToolsRoot 'C:\path\to\VC\Tools\MSVC\version' -WindowsSdkRoot 'C:\Program Files (x86)\Windows Kits\10' -SdkVersion 10.0.22621.0
```

输出在 `artifacts\DeviceBatteryPlugin`，脚本同时运行原生回归测试。覆盖独立设备项目、四设备固定宽度绘制、原插件 ABI、选择规则、空值/0%/异常值、中文 INI、充电/缓存提示、过期文件、死亡进程、PID 重用和恢复。用 `device_battery_tests.exe --live <插件配置目录>` 可读取真实快照，便于部署前检查；实际屏幕仍需核对宿主绘制诊断或界面。

升级前备份原 DLL、插件 INI 及 TrafficMonitor 的 config.ini；停止 TrafficMonitor 后替换 DLL。新增项目会改变插件全局索引，已有手工配置的 item_order 和按项目颜色需要相应重排。
