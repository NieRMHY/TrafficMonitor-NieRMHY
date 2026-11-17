# TrafficMonitor：任务栏监控与多设备电量插件

**主开发目录**：`/home/niermhy/workspace/project/project_myself/github_project/TrafficMonitor-NieRMHY`

**项目文档根**：`/home/niermhy/workspace/Obsidian/Obsidian_Note_Server/工作知识记录/个人开发项目/TrafficMonitor-NieRMHY/`

**独立 GitHub 仓库**：`https://github.com/NieRMHY/TrafficMonitor-NieRMHY.git`

**接力开发及 GitHub 默认分支**：`feature/ferrispulse-device-batteries`

<!-- Add by MHY, 2026-11-17 配置 miniPC 主开发工作区与多端同步规范 -->

## 项目概述

基于 TrafficMonitor 的个人修改版本，在 Windows 悬浮窗和任务栏显示网速、硬件统计及多台外设电量。当前功能分支包含 DeviceBatteryPlugin 与 FerrisPulse 的本机 JSON 快照联动。

## 工作方式与规则适用

- 个人仓库只配置 `origin` 指向 `NieRMHY/TrafficMonitor-NieRMHY`，不配置上游 remote 或自动合并；原项目来源见 README，保留许可证和版权信息。
- miniPC（Linux）是主开发机；此仓库保留完整 Git 历史和远程关联。Windows 原生功能在 Windows 构建与验收。
- 开始任务先读本文件及适用的 `AGENTS.md`，按任务查阅模块文档；不要求每次通读所有协议材料。
- 源码沿用上游命名、目录和语言习惯；公共模板的通用命名规则不用于批量改名。修改标注使用所属语言合法的注释形式。
- 保留上游 README、许可证和技术文档；本机介绍、方案及工作记录放上述 vault 文档根。本次用户授权建立这些介绍和规则。
- 已授权范围内直接完成编辑、验证及所需 Git 操作；提交、推送和发布以当次用户授权为准，已有授权无需重复确认。2026-11-17 已授权将三个项目独立维护并同步这些规则。
- 多端同步以 Git 提交为单位：先检查分支和工作树，再 `git fetch origin`；有对应追踪分支且工作树干净时使用 `git pull --ff-only`。发现分叉或未提交修改时保留现场，禁止覆盖或强推。
- 当前 Windows 来源：`E:\Workspace\codex\TrafficMonitor-NieRMHY`。通过 Git 在两端交换后续代码；不要反复复制整个工作区或覆盖 `.git`。
- 网络沿用环境文档的会话代理。2026-11-17 初始化时既有代理不可用，临时经 Windows SSH 隧道取回 GitHub 数据；临时端口没有写入 Git 配置。后续先核对网络，不写入失效的临时地址。

## 环境与常用命令

- 主程序为 Windows C++ / MFC 工程；电量插件使用 C++17、MSVC x64 与 Windows SDK。
- 主程序和插件必须匹配宿主位数。完整程序需要 Visual Studio C++ 桌面开发组件及 MFC；只构建电量插件时使用脚本指定的 MSVC 和 SDK。
- Linux 主机用于编辑、Git 同步和静态检查；原生构建、任务栏绘制及真实设备联动在 Windows 验证。不能把 Linux 语法检查当作 Windows 运行通过。

在 Windows 仓库根目录构建插件并运行其回归测试；路径按该 Windows 开发机实际安装填写：

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/build_device_battery.ps1 -VCToolsRoot 'C:\path\to\VC\Tools\MSVC\version' -WindowsSdkRoot 'C:\Program Files (x86)\Windows Kits\10' -SdkVersion 10.0.22621.0
```

脚本输出到 `artifacts/DeviceBatteryPlugin/`。完整 Lite 程序的 CI 命令（MSBuild 开发终端）：

```powershell
msbuild TrafficMonitor_Lite.sln -p:configuration="Release (lite)" -p:platform=x64 -p:platformToolset=v143
```

## 架构与核心规则

- `TrafficMonitor/`：主程序、窗口、任务栏与配置；`include/`：插件接口。
- `PluginDemo/DeviceBatteryPlugin/`：设备电量读取、显示与插件实现；先读此目录 `README.md`。
- `tests/device_battery_tests.cpp` 与 `tools/build_device_battery.ps1`：原生回归入口；`.github/workflows/main.yml`：完整程序构建。
- FerrisPulse 是硬件采集方，插件只读 `%LOCALAPPDATA%\FerrisPulse\battery-status.json`；不要在消费侧重新打开 HID。
- 保留原 HTTP 单设备模式、插件 ABI 和原项目 ID。显式选定设备失败时显示 `--`，不得回退为另一台设备。
- 0% 是有效电量；未知、待机、离线、过期及生产进程退出均不能当作实时电量。分别检查心跳和硬件观察时间，保留 PID 重用与恢复处理。
- 保持四设备布局及固定数值列宽，新增显示项目时检查排序、全局索引、配色和双行任务栏。Windows Profile API 使用的中文插件 INI 需 UTF-16 LE BOM。
- 正式运行目录 `D:\Tools\TrafficMonitor` 与源码目录分离。部署 DLL 前备份配置、正常退出宿主；修改源码并不授权覆盖正在使用的程序。
- 主程序实验构建不能替代此前验证的正式运行版本；历史安装配置见 vault 的联动记录。

## 按任务阅读

- 插件契约与验证：`PluginDemo/DeviceBatteryPlugin/README.md`。
- FerrisPulse 生产侧：`../FerrisPulse/docs/TRAFFICMONITOR_INTEGRATION.md`。
- 历史布局、恢复和硬件证据：vault 的 `外设电量与任务栏监控/`。

## 初始化验证与交付状态

- 源码基线：`59202bfdddcd43568fbc702666be88c73ce1124c`（2026-11-17）。具体文件比对与环境限制见 vault `工作记录/2026-11-17_迁入miniPC与源码核验.md`。
- 本次将 README 来源说明和开发规则提交到独立个人仓库；未执行原生程序构建及硬件测试，不能把历史测试结果当作此次验证。

## 公共规则

以下按现有项目约定复制自 vault 根目录 `_project_CLAUDE_template.md`；已确定的文档根与平台边界以上方为准。

## 文档规则
- 项目过程中按需生成记录、说明文档，统一存放在 Obsidian vault
- **Vault 根路径**: `/home/niermhy/workspace/Obsidian/Obsidian_Note_Server/`
- 开始时先检索 vault 目录结构，找到与项目领域匹配的分类目录；无合适分类则在 vault 根目录新建
- 在该分类下创建本项目子目录作为文档根
- 确定文档路径后，将完整路径写入本文档顶部，后续直接使用无需重复检索
- 生成/更新/读取文档 → 均从 vault 对应路径操作
- 写入与生成文档时遵循 vault 根目录 `CLAUDE.md` 的整理规则
- 使用 `[[内部链接]]` 在文档之间交叉引用

### 工作记录
- 中间过程记录、阶段总结、plan 方案均写入本项目文档目录下的 `工作记录/` 子目录
- 记录文档命名：`YYYY-MM-DD_简短描述.md`
- Plan 文档命名：`_Plan_描述.md`
- 已完成的 plan 在正文开头标注完成日期

## 工作区环境汇总
- 阅读 `/home/niermhy/workspace/environment/README.md`（含服务器连接、SDK 共享、烧写等索引）
- environment 文档变更后运行 `env-sync` 同步 vault 镜像（`信息记录/environment镜像/`）

## 语言
- 沟通使用简体中文，代码标识符使用英文
- 日志信息：优先使用中文，或英文

## 网络环境
- 当前环境：国内网络，GitHub 直连不稳定
- 需要访问外部网络时先 `proxy-on`（`~/scripts/ConfigProxy.sh`，结束后可 `proxy-off`）

## 命名规范
- 函数/类名：PascalCase（大驼峰）
- 变量名：camelCase（小驼峰）
- 文件名：camelCase（小驼峰），少用连字符或下划线

## 代码风格
- 不写多余注释，只写"为什么"而非"是什么"
- 修改代码时添加标注：`# Add by MHY, <说明>` 或 `# Modify by MHY, <说明>`
- 不引入不必要的抽象，三行相似代码好过一次过早的封装
- 只处理真实发生的场景，不为"可能"做防御性编程
- 不写 docstring、不写 README（项目记录/Plan 写入 vault 工作记录目录，不在此列）

## 工具使用
- 优先使用 Read/Edit/Write 工具而非 cat/sed/awk
- 独立操作并行执行（多个 Bash 调用或 Agent 调用同时发出）
- git 操作需确认，不做 force push / reset --hard 等破坏性操作
- 提交时作者使用当前 git config，不添加 Co-Authored-By（除非明确要求）
