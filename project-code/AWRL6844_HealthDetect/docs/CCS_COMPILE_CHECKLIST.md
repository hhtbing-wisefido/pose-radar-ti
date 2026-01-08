# 🚀 CCS编译前配置检查清单

> **在CCS中导入和编译项目前，请仔细阅读此文档！**

---

## ✅ 已修复的问题

### 1. SDK路径配置 ✅
**问题**: 原来硬编码为 `C:/ti/mmwave_sdk_03_06_02_00_00`  
**修复**: 改用CCS环境变量 `${COM_TI_MMWAVE_SDK_INSTALL_DIR}`  
**影响**: 现在可以自动检测SDK安装路径

### 2. 系统打包路径 ✅
**问题**: makefile中的路径引用错误  
**修复**: 使用 `${CCS_PROJECT_DIR}` 动态路径  
**影响**: 打包脚本现在可以正确找到输出文件

---

## 🔴 CCS导入前必须配置

### 1. SDK环境变量

**CCS必须能找到mmWave SDK！**

#### 方法1: 使用CCS的Product Discovery（推荐）

```
CCS → Window → Preferences → Code Composer Studio → Products
→ 确认 "mmWave SDK" 已被检测到
→ 路径应该类似: C:/ti/mmwave_sdk_03_06_02_00_00
```

#### 方法2: 手动设置环境变量

在导入项目前，在Windows中设置：
```
变量名: COM_TI_MMWAVE_SDK_INSTALL_DIR
变量值: C:/ti/mmwave_sdk_03_06_02_00_00  (你的实际SDK路径)
```

### 2. RF固件路径验证

**必须验证SDK中RF固件存在！**

检查以下路径是否存在文件：
```
${SDK}/firmware/mmwave_dfp/rfsfirmware/xWRL68xx/mmwave_rfs_patch.rig
```

如果不存在，需要在metaimage_cfg.json中更新路径。

---

## 📋 CCS导入和编译步骤

### Step 1: 导入项目

```
CCS → Project → Import CCS Projects
→ Select search-directory: 
   D:\7.project\TI_Radar_Project\project-code\AWRL6844_HealthDetect
→ 发现3个项目:
   ✅ AWRL6844_HealthDetect_MSS
   ✅ AWRL6844_HealthDetect_DSS
   ✅ AWRL6844_HealthDetect_System
→ Select All → Finish
```

### Step 2: 验证项目配置

**检查MSS项目：**
```
右键 AWRL6844_HealthDetect_MSS → Properties
→ Build → Variables
→ 确认 TI_SDK_ROOT 变量被正确解析
```

**检查DSS项目：**
```
右键 AWRL6844_HealthDetect_DSS → Properties
→ Build → Variables
→ 确认 TI_SDK_ROOT 变量被正确解析
```

### Step 3: 分别编译（可选，用于调试）

```
1. Build AWRL6844_HealthDetect_MSS (Release)
   → 应该生成: AWRL6844_HealthDetect_MSS/Release/AWRL6844_HealthDetect_MSS.out

2. Build AWRL6844_HealthDetect_DSS (Release)
   → 应该生成: AWRL6844_HealthDetect_DSS/Release/AWRL6844_HealthDetect_DSS.out
```

### Step 4: 系统打包编译（🔴 正式方式）

```
Build AWRL6844_HealthDetect_System (Release)

流程:
1. 自动编译MSS → MSS.out
2. 自动编译DSS → DSS.out
3. 执行后处理 → makefile_system_ccs_bootimage_gen
4. 生成 .appimage
   → AWRL6844_HealthDetect_System/Release/health_detect_system.release.appimage
```

---

## ⚠️ 预期的编译警告

### 正常的警告（可以忽略）

#### 1. TODO注释相关警告
```
warning: TODO comments in code
说明: 这是Milestone 1的设计，代码框架完整但功能待实现
影响: 不影响编译
```

#### 2. 未使用的变量
```
warning: unused variable 'xxx'
说明: 骨架代码中定义了变量但TODO部分未实现
影响: 不影响编译
```

#### 3. 函数返回值警告
```
warning: function may return without value
说明: TODO函数返回语句未实现
影响: 不影响编译，但运行时可能有问题
```

### 🔴 必须修复的错误

#### 1. SDK路径错误
```
error: cannot find file 'xxx.h'
原因: SDK环境变量未设置或路径错误
解决: 检查 ${COM_TI_MMWAVE_SDK_INSTALL_DIR}
```

#### 2. 链接脚本错误
```
error: cannot open linker command file
原因: linker_mss.cmd 或 linker_dss.cmd 有问题
解决: 检查 src/system/ 目录下的文件
```

#### 3. 打包脚本错误
```
error: cannot find MSS.out or DSS.out
原因: 输出文件路径不对
解决: 检查编译输出目录
```

---

## 🐛 常见问题排查

### 问题1: SDK变量未解析

**症状**: 编译错误，找不到头文件  
**原因**: `${COM_TI_MMWAVE_SDK_INSTALL_DIR}` 未定义  
**解决**:
```
1. CCS → Window → Preferences → Code Composer Studio → Products
2. 确认mmWave SDK已安装并被检测
3. 重启CCS
4. 重新导入项目
```

### 问题2: 系统打包失败

**症状**: System项目编译失败  
**原因**: MSS或DSS编译失败，或makefile路径错误  
**解决**:
```
1. 先单独编译MSS和DSS验证
2. 检查输出目录是否有.out文件
3. 检查makefile中的路径变量
4. 查看Console输出的详细错误
```

### 问题3: RF固件找不到

**症状**: .appimage生成失败，提示找不到RF固件  
**原因**: mmwave_rfs_patch.rig路径错误  
**解决**:
```
1. 在SDK目录搜索: mmwave_rfs_patch.rig
2. 更新metaimage_cfg.*.json中的路径
3. 确保使用SDK的相对路径或绝对路径
```

---

## 📊 编译成功的验证

### 检查输出文件

```
✅ MSS编译成功:
   AWRL6844_HealthDetect_MSS/Release/AWRL6844_HealthDetect_MSS.out
   大小: ~300-500 KB

✅ DSS编译成功:
   AWRL6844_HealthDetect_DSS/Release/AWRL6844_HealthDetect_DSS.out
   大小: ~200-300 KB

✅ 系统打包成功:
   AWRL6844_HealthDetect_System/Release/health_detect_system.release.appimage
   大小: ~700-800 KB
   
   包含:
   - MSS Image (~500KB)
   - DSS Image (~200KB)
   - RF Firmware (~50KB)
```

### 验证.appimage内容

使用TI工具验证：
```bash
cd ${SDK}/tools/MetaImageGen
metaImage_creator.exe --verify path/to/health_detect_system.release.appimage

预期输出:
✅ Meta Header: Valid
✅ MSS Image: Valid
✅ DSS Image: Valid
✅ RF Firmware: Valid
```

---

## 🎯 编译后的下一步

### Milestone 2验证清单

- [ ] MSS项目编译成功
- [ ] DSS项目编译成功
- [ ] System项目打包成功
- [ ] 生成.appimage文件
- [ ] .appimage通过验证工具检查
- [ ] （可选）使用UniFlash烧录测试

### ⚠️ 重要提醒

**Milestone 1完成 ≠ 代码可运行！**

当前状态：
- ✅ 架构重建完成
- ✅ 编译配置正确
- ✅ 可以成功编译
- ❌ 功能未实现（TODO）
- ❌ 不能在硬件上运行

Milestone 2目标：
- 验证项目结构正确
- 验证编译系统工作
- 验证系统打包成功
- **不期望**功能运行

Milestone 3-4才会实现功能。

---

## 📚 参考文档

- **系统打包详细说明**: `src/system/README.md`
- **编译指南**: `docs/BUILD_GUIDE.md`
- **架构说明**: `README.md`
- **进度报告**: `docs/架构重建进度报告.md`

---

> 🔴 **关键提醒**: 这是**架构重建**项目，不是**功能完整**项目！
> 
> 编译成功 = Milestone 1 + 2 完成 ✅  
> 功能运行 = Milestone 3 + 4（未开始）❌
