# AWRL6844 Health Detection - CCS Project Build Guide

## 项目结构

```
AWRL6844_HealthDetect/
├── mss_project.projectspec    # MSS/R5F项目配置
├── dss_project.projectspec    # DSS/C66x项目配置
└── src/
    ├── common/                 # 共享接口层
    ├── mss/                    # MSS应用层
    ├── dss/                    # DSS算法层
    └── system/                 # 系统配置（链接脚本）
```

## 编译步骤

### 1. 导入项目到CCS

#### 方法A: 通过Project Explorer

1. 打开CCS (Code Composer Studio)
2. `File` → `Import...`
3. 选择 `CCS Projects`
4. Browse到 `AWRL6844_HealthDetect` 目录
5. 勾选 `AWRL6844_HealthDetect_MSS` 和 `AWRL6844_HealthDetect_DSS`
6. 点击 `Finish`

#### 方法B: 通过命令行

```batch
cd /d D:\7.project\TI_Radar_Project\project-code\AWRL6844_HealthDetect

REM 导入MSS项目
eclipsec.exe -noSplash -data "C:\ti\ccs\workspace" ^
  -application com.ti.ccstudio.apps.projectImport ^
  -ccs.location "%cd%\mss_project.projectspec"

REM 导入DSS项目
eclipsec.exe -noSplash -data "C:\ti\ccs\workspace" ^
  -application com.ti.ccstudio.apps.projectImport ^
  -ccs.location "%cd%\dss_project.projectspec"
```

### 2. 配置SDK路径

**重要**: 修改项目配置中的SDK路径

编辑 `.projectspec` 文件中的 `TI_SDK_ROOT` 变量：

```xml
<pathVariable name="TI_SDK_ROOT" path="C:/ti/mmwave_sdk_03_06_02_00_00" scope="project"/>
```

改为你的实际SDK安装路径。

### 3. 编译MSS项目

1. 在Project Explorer中选择 `AWRL6844_HealthDetect_MSS`
2. 右键 → `Build Project`
3. 或使用快捷键 `Ctrl+B`

**预期输出**:
```
Building file: src/mss/health_detect_main.c
Building file: src/mss/dpc_control.c
...
Finished building target: AWRL6844_HealthDetect_MSS.out
```

### 4. 编译DSS项目

1. 在Project Explorer中选择 `AWRL6844_HealthDetect_DSS`
2. 右键 → `Build Project`

**预期输出**:
```
Building file: src/dss/dss_main.c
Building file: src/dss/feature_extract.c
...
Finished building target: AWRL6844_HealthDetect_DSS.out
```

## 编译验证

### 成功标志

✅ **MSS编译成功**:
- 生成 `Debug/AWRL6844_HealthDetect_MSS.out`
- 生成 `Debug/AWRL6844_HealthDetect_MSS.map`
- 0 errors, 允许有warnings

✅ **DSS编译成功**:
- 生成 `Debug/AWRL6844_HealthDetect_DSS.out`
- 生成 `Debug/AWRL6844_HealthDetect_DSS.map`
- 0 errors, 允许有warnings

### 常见编译问题

#### 问题1: "undefined reference to MMWave_init"

**原因**: mmWave SDK库未正确链接

**解决**:
```xml
<!-- 在.projectspec中确认库路径 -->
<linkerBuildOptions>
    -i${TI_SDK_ROOT}/packages/ti/control/mmwave/lib
    -llibmmwave_xwr68xx.ae674
</linkerBuildOptions>
```

#### 问题2: "cannot find linker_mss.cmd"

**原因**: 链接脚本路径错误

**解决**: 确认 `src/system/linker_mss.cmd` 存在

#### 问题3: "L3 RAM section overlap"

**原因**: 共享内存配置冲突

**解决**: 检查 `src/system/shared_memory.ld` 中的地址定义

## 下一步

编译成功后，可以进行：

1. **固件烧录**: 使用UniFlash烧录 `.out` 文件到雷达
2. **调试**: 在CCS中使用XDS仿真器调试
3. **功能测试**: 验证雷达启动、DPC运行、数据输出

## 重要提醒

⚠️ **当前状态**: 框架代码，包含TODO标记

需要完成的部分：
- [ ] mmWave API实际调用（radar_control.c）
- [ ] IPC mailbox实现（dpc_control.c, dss_main.c）
- [ ] BIOS配置文件（.cfg）
- [ ] ADC数据路径配置

**本次目标**: 验证架构可编译，不是运行固件！

---

> 📝 **注意**: 这是从零重建的新架构，参考了mmw_demo但代码全新编写。
