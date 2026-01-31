# 📦 EXE 发布管理规范

> 🎯 **目的**：统一管理编译后的 EXE 文件，使用版本号命名，避免混乱

---

## 📁 目录结构

```
项目文档/3-固件工具/
├── 01-AWRL6844固件系统工具/
│   ├── 5-Scripts/
│   │   ├── flash_tool.py              # 源代码（VERSION = "2.4.4"）
│   │   └── build_release.ps1          # 🆕 自动化编译脚本
│   └── release/                       # 🆕 发布目录
│       └── AWRL6844_固件系统工具_v2.4.4.exe
│
└── 05-雷达配置参数研究/
    ├── radar_test_gui.py              # 源代码（v1.1.5）
    ├── build_release.ps1              # 🆕 自动化编译脚本
    └── release/                       # 🆕 发布目录
        └── 雷达配置参数测试工具_v1.1.5.exe
```

---

## 🚀 使用方法

### 方法1：使用自动化脚本（推荐）⭐

#### 固件系统工具

```powershell
# 进入脚本目录
cd "D:\7.project\TI_Radar_Project\项目文档\3-固件工具\01-AWRL6844固件系统工具\5-Scripts"

# 控制台模式（便于调试）
.\build_release.ps1

# 窗口模式（无控制台）
.\build_release.ps1 -NoConsole
```

#### 雷达配置工具

```powershell
# 进入项目目录
cd "D:\7.project\TI_Radar_Project\项目文档\3-固件工具\05-雷达配置参数研究"

# 控制台模式（便于调试）
.\build_release.ps1

# 窗口模式（无控制台）
.\build_release.ps1 -NoConsole
```

**自动化脚本的优点**：
- ✅ 自动读取版本号（从源代码中提取）
- ✅ 自动生成带版本号的文件名
- ✅ 自动输出到 `release/` 目录
- ✅ 显示详细的编译信息（大小、耗时等）
- ✅ 编译失败时自动报错

---

### 方法2：手动 PyInstaller 命令

#### 固件系统工具

```powershell
cd "01-AWRL6844固件系统工具\5-Scripts"

# 手动指定版本号
$version = "2.4.4"
pyinstaller --onefile --clean `
    --name "AWRL6844_固件系统工具_v$version" `
    --distpath="..\release" `
    --workpath="..\build" `
    --specpath="..\build" `
    --icon="..\radar_icon.ico" `
    flash_tool.py
```

#### 雷达配置工具

```powershell
cd "05-雷达配置参数研究"

# 手动指定版本号
$version = "1.1.5"
pyinstaller --onefile --clean `
    --name "雷达配置参数测试工具_v$version" `
    --distpath="release" `
    --workpath="build" `
    --specpath="build" `
    --icon="radar_icon.ico" `
    radar_test_gui.py
```

---

## 📝 版本号管理

### 固件系统工具版本号位置

**文件**：`01-AWRL6844固件系统工具\5-Scripts\flash_tool.py`

```python
# 在文件顶部找到版本号定义
VERSION = "2.4.4"  # ← 修改这里
```

### 雷达配置工具版本号位置

**文件**：`05-雷达配置参数研究\radar_test_gui.py`

```python
# 在窗口标题中找到版本号
self.root.title("🔬 雷达配置参数测试工具 v1.1.5 - 双端口模式")
#                                            ^^^^^^ ← 修改这里
```

**注意**：雷达配置工具的版本号在窗口标题中，编译脚本会自动提取

---

## 🔄 版本发布流程

### 步骤1：更新版本号

**固件系统工具**：
```python
# flash_tool.py
VERSION = "2.4.5"  # 从 2.4.4 更新到 2.4.5
```

**雷达配置工具**：
```python
# radar_test_gui.py
self.root.title("🔬 雷达配置参数测试工具 v1.1.6 - 双端口模式")
#                                            ^^^^^^ 从 v1.1.5 更新到 v1.1.6
```

### 步骤2：运行编译脚本

```powershell
# 固件系统工具
cd "01-AWRL6844固件系统工具\5-Scripts"
.\build_release.ps1

# 雷达配置工具
cd "05-雷达配置参数研究"
.\build_release.ps1
```

### 步骤3：验证输出

编译完成后，检查 `release/` 目录：

```
✅ release/AWRL6844_固件系统工具_v2.4.5.exe
✅ release/雷达配置参数测试工具_v1.1.6.exe
```

### 步骤4：测试新版本

运行新编译的 EXE，验证功能正常。

### 步骤5：（可选）删除旧版本

```powershell
# 如果新版本测试通过，可以删除旧版本
Remove-Item "release\AWRL6844_固件系统工具_v2.4.4.exe"
Remove-Item "release\雷达配置参数测试工具_v1.1.5.exe"
```

---

## 📊 编译脚本功能说明

### 自动化功能

1. **自动读取版本号**
   - 固件工具：从 `VERSION = "x.x.x"` 提取
   - 雷达工具：从窗口标题 `v1.1.5` 提取

2. **自动生成文件名**
   - 格式：`工具名称_vX.X.X.exe`
   - 示例：`AWRL6844_固件系统工具_v2.4.4.exe`

3. **自动输出到 release**
   - 所有编译产物都放在 `release/` 目录
   - 临时文件放在 `build/` 目录

4. **详细信息显示**
   ```
   ✅ 编译成功！
   📦 输出文件:
      📁 位置: D:\...\release
      📄 文件: AWRL6844_固件系统工具_v2.4.4.exe
      📏 大小: 10.32 MB
      📅 时间: 2026/01/29 14:52:21
      ⏱️ 耗时: 45.67 秒
   ```

### 可选参数

- `-NoConsole`：编译为窗口模式（无控制台）
  - 适用于：最终发布版本
  - 不推荐：开发调试阶段（看不到调试信息）

---

## 🛠️ 故障排查

### 问题1：编译失败，提示找不到 pyinstaller

**解决方案**：
```powershell
pip install pyinstaller
```

### 问题2：编译成功但无法运行

**检查清单**：
1. 是否有杀毒软件拦截？
2. 是否缺少运行时依赖？
3. 查看控制台输出（使用控制台模式编译）

### 问题3：版本号未自动更新

**检查**：
- 确认源代码中的版本号已更新
- 使用编译脚本而非手动 pyinstaller 命令
- 检查脚本是否正确提取版本号

### 问题4：release 目录中有多个版本

**处理**：
- 测试新版本是否正常
- 正常后删除旧版本
- 保留最近2-3个版本作为回退备份（可选）

---

## 📋 清理临时文件

编译过程会生成临时文件，可以定期清理：

```powershell
# 固件系统工具
cd "01-AWRL6844固件系统工具"
Remove-Item "build" -Recurse -Force -ErrorAction SilentlyContinue

# 雷达配置工具
cd "05-雷达配置参数研究"
Remove-Item "build" -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item "__pycache__" -Recurse -Force -ErrorAction SilentlyContinue
```

**注意**：不要删除 `release/` 目录！

---

## ✅ 最佳实践

1. **始终使用编译脚本**
   - ✅ `.\build_release.ps1`
   - ❌ 不要手动 pyinstaller 命令

2. **版本号规范**
   - 格式：`vX.Y.Z`
   - X：主版本号（重大更新）
   - Y：次版本号（新功能）
   - Z：修订号（bug修复）

3. **开发调试**
   - 使用控制台模式（默认）
   - 查看调试日志

4. **最终发布**
   - 使用窗口模式（`-NoConsole`）
   - 充分测试后再发布

5. **版本管理**
   - 每次发布前更新版本号
   - 在 Git 中打 tag：`v2.4.4`
   - 保留最近几个版本的 EXE

---

## 📞 问题反馈

如果遇到问题：
1. 检查本文档的故障排查部分
2. 查看编译脚本的输出信息
3. 检查 PyInstaller 日志（`build/` 目录）

---

> 🎯 **记住**：每次编译使用 `.\build_release.ps1`，版本号自动管理！

> 📅 **更新**：2026/01/29 - 初始版本
