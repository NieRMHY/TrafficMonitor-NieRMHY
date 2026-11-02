# 多设备电量插件

保留原 HTTP API 单设备用法，并增加 FerrisPulse 本机快照和最多 8 个独立显示项目。未配置 `snapshot_file` 时继续使用旧 HTTP 设置，默认只有一个项目。

## FerrisPulse 配置

FerrisPulse 需要包含 `BatterySnapshotExporter`，并在当前用户 `HKCU\Software\RazerBatteryTray` 设置 DWORD `ExportBatteryStatus=1`。正常轮询后生成 `%LOCALAPPDATA%\FerrisPulse\battery-status.json`。

在 TrafficMonitor 的配置目录 `plugins\DeviceBatteryPlugin.dll.ini` 写入以下内容。**包含中文时请保存为 UTF-16 LE BOM**，与 Windows Profile API 一致：

```ini
[api]
snapshot_file=%LOCALAPPDATA%\FerrisPulse\battery-status.json
refresh_interval_ms=3000
max_snapshot_age_seconds=900
[display]
item_count=3
trailing_spacer=1
[device0]
source=Razer
label_text=雷蛇
[device1]
source=Dareu
label_text=AE7
[device2]
source=Maono
label_text=MIC
```

重启 TrafficMonitor，在显示项目中启用三个电量项。项目 ID 依次为 `Device24GBattery`、`Device24GBattery_2`、`Device24GBattery_3`，原项目 ID 保留。

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
| MIC 电量 | 空白 |

只在“雷蛇/AE7”标签前留一处设备组间距，MIC 与两台鼠标采用正常列间距。CPU 与独显的功耗、温度标签在 `config.ini` 的 `[plugin_display_str_taskbar_window]` 对应项目设为空字符串。新增占位项目后，手工保存的全局索引与配色需将后续 HardwareMonitor 项目后移一位。

整组右移只使用用户配置 `[task_bar] window_offset_left=70`，不修改定位代码。摄氏度保持与数值相同字号，HardwareMonitor.ini 的温度项目使用 `unit = " °C"`（单位前为 U+2009 细空格）略微拉开间距。

NieRMHY 的任务栏配置将这三个项目放在固态温度后面，并使用固定设备配色：

| 项目 | 标签 | RGB 颜色 |
| --- | --- | --- |
| Device24GBattery | 雷蛇 | 深绿 `#3CC878` |
| Device24GBattery_2 | AE7 | 粉色 `#F49AC2` |
| Device24GBattery_3 | MIC | 橙色 `#FFB454` |

这些颜色由 TrafficMonitor 的每项目颜色设置控制。FerrisPulse 自身使用按剩余电量变化的绿/黄/红规则，两边共享数值，显示配色分别设置。

选择优先级为 `device_id`、`device_name`、`source`，显式选择不存在时显示 `--`，不会退回另一台设备。多个同品牌设备请用快照内 `id` 配置 `device_id`。未指定选择条件时保留旧的托盘设备优先策略。

`77%+` 是确认充电。设备休眠/离线、快照过期、生产进程退出、数据未知或文件不可读时显示 `--`，不把历史电量显示成当前数据。文件最大 256 KiB；生产 PID 必须存活且进程创建时间不得晚于文件时间。读取每项时不会触碰 HID；实际更新频率沿用 FerrisPulse 的硬件轮询设置。900 秒过期阈值可设为 30–3600 秒。

## 构建和测试

使用 MSVC x64 工具链和 Windows SDK，在仓库根目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_device_battery.ps1 -VCToolsRoot 'C:\path\to\VC\Tools\MSVC\version' -WindowsSdkRoot 'C:\Program Files (x86)\Windows Kits\10' -SdkVersion 10.0.22621.0
```

输出在 `artifacts\DeviceBatteryPlugin`，脚本同时运行原生回归测试。覆盖三个独立项目、原插件 ABI、选择规则、空值/0%/异常值、中文 INI、充电/缓存提示、过期文件、死亡进程、PID 重用和恢复。用 `device_battery_tests.exe --live <插件配置目录>` 可读取真实快照并打印三个项目，便于部署前检查。

升级前备份原 DLL、插件 INI 及 TrafficMonitor 的 config.ini；停止 TrafficMonitor 后替换 DLL。新增项目会改变插件全局索引，已有手工配置的 item_order 和按项目颜色需要相应重排。
