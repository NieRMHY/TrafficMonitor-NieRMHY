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

选择优先级为 `device_id`、`device_name`、`source`，显式选择不存在时显示 `--`，不会退回另一台设备。多个同品牌设备请用快照内 `id` 配置 `device_id`。未指定选择条件时保留旧的托盘设备优先策略。

`77%+` 是确认充电。设备休眠/离线、快照过期、生产进程退出、数据未知或文件不可读时显示 `--`，不把历史电量显示成当前数据。文件最大 256 KiB；生产 PID 必须存活且进程创建时间不得晚于文件时间。读取每项时不会触碰 HID；实际更新频率沿用 FerrisPulse 的硬件轮询设置。900 秒过期阈值可设为 30–3600 秒。

## 构建和测试

使用 MSVC x64 工具链和 Windows SDK，在仓库根目录执行：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_device_battery.ps1 -VCToolsRoot 'C:\path\to\VC\Tools\MSVC\version' -WindowsSdkRoot 'C:\Program Files (x86)\Windows Kits\10' -SdkVersion 10.0.22621.0
```

输出在 `artifacts\DeviceBatteryPlugin`，脚本同时运行原生回归测试。覆盖三个独立项目、原插件 ABI、选择规则、空值/0%/异常值、中文 INI、充电/缓存提示、过期文件、死亡进程、PID 重用和恢复。用 `device_battery_tests.exe --live <插件配置目录>` 可读取真实快照并打印三个项目，便于部署前检查。

升级前备份原 DLL、插件 INI 及 TrafficMonitor 的 config.ini；停止 TrafficMonitor 后替换 DLL。新增项目会改变插件全局索引，已有手工配置的 item_order 和按项目颜色需要相应重排。
