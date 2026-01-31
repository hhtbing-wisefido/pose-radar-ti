# 🧰 Release.EXE 提示词模板（可复制复用）

> 🎯 目标：**“PY 与 EXE 功能/行为完全一致（不能缺胳膊少腿）”**，并输出带版本号的 `onefile` EXE 到项目根 `release/`。

---

## ✅ 一句话需求（给 AI 的任务描述）

请对以下工具做发布打包，要求“完整无误不出错”：

- 📁 目标项目/工具目录：`[填：绝对路径]`
- 🧩 入口脚本：`[填：入口 .py]`
- 🏷️ 版本号：`[填：vX.Y.Z]`
- 🧾 EXE 文件名：`[填：ToolName_vX.Y.Z.exe]`
- 🎨 图标：`[填：.ico 路径]`
- 📦 输出目录：项目根 `release/`

并且必须通过“干净目录 + 进程取证”验收，证明：

- 🟰 `python 入口脚本.py` 与 `release/*.exe` 的**启动流程、单例行为、UI与功能、默认路径、日志输出**一致
- 🧵 若工具采用 `--detach`：必须满足 **启动器退出后，仅保留一个 GUI 进程**（按取证判定）

---

## 🔒 强制约束（不满足即视为失败）

### 🧩 PY = EXE（行为级一致）

- ✅ **同一套逻辑**：不要出现“脚本能用、EXE 缺功能/缺资源/缺导入”的情况
- ✅ 任何 import、资源路径、配置文件、图片、cfg/firmware 等，EXE 里必须可用
- ❌ 禁止“先简化/先删功能保证能跑”
- ❌ 禁止新增 stub/mock 代替真实逻辑

### 🪟 Windows 图标（任务栏/标题栏稳定）

- ✅ Tkinter：`SetCurrentProcessExplicitAppUserModelID` 必须在 `tk.Tk()` 之前
- ✅ `iconphoto(256)` 且保持强引用（避免回退默认 Python 图标）
- ✅ 打包图标：PyInstaller `--icon` / spec `icon=` 一致

### 📦 PyInstaller onefile（发布形态固定）

- ✅ 必须 `onefile`、`--noconsole`（GUI 工具）
- ✅ EXE 文件名必须带版本号，输出到 `release/`
- ✅ 资源必须通过 `_MEIPASS`/exe 同级候选路径可靠加载

---

## 🧠 代码侧“必做清单”（防止 onefile 行为漂移）

### 🚀 `--detach` 启动链路（如果工具采用后台 GUI 模式）

要求链路固定为：

1) 启动器（无 `--detach`）启动
2) 单例检查（只在启动器阶段做）
3) 启动 `--detach` 子进程（真正 GUI 进程）
4) 启动器必须立刻退出（命令行返回）
5) 最终仅保留一个 GUI 窗口（并按取证判定是否仅 1 个 GUI 进程）

**实现要点（onefile 下尤其关键）**：

- ✅ 启动子进程时清理 PyInstaller 相关环境变量（例如 `_MEIPASS2`、`_PYI_*`）
- ✅ 设置 `PYINSTALLER_RESET_ENVIRONMENT=1`
- ✅ Windows `creationflags` 建议包含：
	- `CREATE_NEW_PROCESS_GROUP`
	- `DETACHED_PROCESS`
	- `CREATE_NO_WINDOW`
	- `CREATE_BREAKAWAY_FROM_JOB`
- ✅ 启动器退出用 `os._exit(0)`（避免父进程常驻）

### 🧷 单例检查（onefile 父子同名陷阱）

- ✅ 单例检查必须排除：`current_pid`、`parent_pid`
- ✅ 单例弹框行为：与脚本版一致（同文案、同按钮、同默认行为）

### 🧰 资源定位（脚本/EXE 双态一致）

- ✅ 优先 `_MEIPASS`（frozen）
- ✅ 其次 exe 同级目录（便于外置资源）
- ✅ 再回退脚本目录（dev）

---

## 🏗️ 打包要求（spec/命令二选一，但必须可复现）

### ✅ 推荐：spec 固化（更稳定）

- 📄 spec 路径：`temp/pyinstaller/spec/[ToolName]_vX.Y.Z.spec`
- ✅ 明确写入：入口、datas、hiddenimports、icon、name、distpath=`release/`

### 🧾 允许：命令行（必须把完整命令写入文档/记录）

- ✅ `--onefile --clean --noconsole --icon ... --add-data ...`
- ✅ 输出到 `release/`

---

## 🧪 验收：干净目录 + 进程取证（必须输出证据文件）

### 📁 干净目录要求

- ✅ 在 `temp/exe_smoke/[ToolShortName]/` 里测试（该目录不放源码、不放资源）
- ✅ 仅复制 `release/[ToolName]_vX.Y.Z.exe` 到该目录运行

### 🔎 取证脚本（PowerShell，可直接复制运行）

将 `[EXE_NAME]` 替换为实际 EXE 文件名（例如 `RadarConfigTestTool_v1.1.5.exe`）：

```powershell
$ErrorActionPreference = 'Stop'

$exeName = '[EXE_NAME]'
$outFile = Join-Path $PWD ('proc_evidence_' + ($exeName -replace '\\W','_') + '.txt')

function Snap([string]$tag) {
	$procs = Get-CimInstance Win32_Process |
		Where-Object { $_.Name -ieq $exeName } |
		Select-Object Name, ProcessId, ParentProcessId, CommandLine

	Add-Content -Path $outFile -Value ('\n==== ' + $tag + ' ====' )
	if (-not $procs) {
		Add-Content -Path $outFile -Value 'NO_MATCHING_PROCESS'
		return
	}

	$procs | Format-Table -AutoSize | Out-String | Add-Content -Path $outFile
}

Add-Content -Path $outFile -Value ('RUN_AT=' + (Get-Date).ToString('s'))
Add-Content -Path $outFile -Value ('CWD=' + $PWD)

Start-Process -FilePath (Join-Path $PWD $exeName)

Start-Sleep -Seconds 2;  Snap '+2s'
Start-Sleep -Seconds 8;  Snap '+10s'
Start-Sleep -Seconds 20; Snap '+30s'
```

### ✅ 取证判定标准（写清楚 PASS/FAIL）

你必须在证据文件末尾给出机器可读的结论，例如：

- `PASS_only_detach_after_30s=True/False`
- `PASS_single_instance_popup_behavior=True/False`
- `PASS_ui_function_parity=True/False`

并解释失败原因与修复点（基于代码与取证，不允许推测）。

---

## 📦 最终交付（必须列出清单）

- ✅ `release/[ToolName]_vX.Y.Z.exe`
- ✅ 对应 spec（如使用）：`temp/pyinstaller/spec/[ToolName]_vX.Y.Z.spec`
- ✅ 取证文件：`temp/exe_smoke/[ToolShortName]/proc_evidence_*.txt`

---

## 🧩 补充：适用于本项目的两个典型工具

- `01`（固件系统工具）：入口通常是 `项目文档/3-固件工具/01-AWRL6844固件系统工具/5-Scripts/flash_tool.py`
- `05`（雷达配置参数研究）：入口通常是 `项目文档/3-固件工具/06-雷达配置参数研究/radar_test_gui.py`

> 📌 提醒：如果 onefile 下出现“父/子进程同名导致单例误判、或父进程常驻”，必须用上面的取证脚本定位，并按“代码侧必做清单”修复。
