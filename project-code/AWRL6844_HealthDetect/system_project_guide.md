# AWRL6844_HealthDetect 系统项目配置指南

## 📦 为什么需要系统项目？

**当前方式**（分别编译）：
- 开发阶段OK，但发布时需要两个.out文件
- 烧录复杂，容易出错

**系统项目方式**（.appimage）：
- 一个文件包含MSS + DSS + RF固件
- 一次烧录完成
- 版本一致性保证

---

## 🎯 添加系统项目的步骤

### Step 1: 创建system目录

```
AWRL6844_HealthDetect/
├── src/
│   ├── common/
│   ├── mss/
│   ├── dss/
│   └── system/          ← 新增
│       ├── system_project.projectspec
│       ├── system.xml
│       ├── makefile_system_ccs_bootimage_gen
│       └── config/
│           └── metaimage_cfg.release.json
```

### Step 2: 创建 system_project.projectspec

```xml
<?xml version="1.0" encoding="UTF-8"?>
<projectSpec>
    <import spec="../mss_project.projectspec"/>
    <import spec="../dss_project.projectspec"/>
    <project
        name="AWRL6844_HealthDetect_System"
        outputType="system"
        toolChain="TICLANG"
        device="Cortex R.AWRL68xx">
        
        <file path="system.xml" action="copy"/>
        <file path="makefile_system_ccs_bootimage_gen" action="copy"/>
        <file path="config/metaimage_cfg.release.json" action="copy"/>
    </project>
</projectSpec>
```

### Step 3: 创建 system.xml

```xml
<?xml version="1.0" encoding="UTF-8"?>
<system>
    <!-- MSS项目 -->
    <project id="project_0" name="AWRL6844_HealthDetect_MSS">
    </project>
    <core id="Cortex_R5_0" project="project_0"/>
    
    <!-- DSS项目 -->
    <project id="project_1" name="AWRL6844_HealthDetect_DSS">
    </project>
    <core id="C66xx_DSP" project="project_1"/>
    
    <!-- 编译后打包 -->
    <postBuildSteps>
        <step command="$(MAKE) -f makefile_system_ccs_bootimage_gen PROFILE=${ConfigName}"/>
    </postBuildSteps>
</system>
```

### Step 4: 创建 makefile_system_ccs_bootimage_gen

参考InCabin_Demos的makefile，调用`metaImage_creator`工具打包。

### Step 5: 创建 metaimage_cfg.release.json

定义打包配置：
- MSS.rig路径
- DSS.rig路径
- RF固件路径
- 输出.appimage路径

---

## 🔧 使用系统项目

### 在CCS中：

```
1. Import System Project
   File → Import → CCS Projects
   Select: src/system/system_project.projectspec

2. Build System Project
   右键点击 "AWRL6844_HealthDetect_System" → Build
   
   自动执行：
   ├─ Build MSS
   ├─ Build DSS
   └─ Package to .appimage

3. 输出文件
   └─ AWRL6844_HealthDetect_System.release.appimage
```

### 烧录：

```
UniFlash:
└─ 选择 .appimage 文件
└─ 一次性烧录所有内容
```

---

## 📊 对比总结

| 特性 | 分别编译 | 系统项目 |
|-----|---------|---------|
| 编译次数 | 2次 | 1次（自动） |
| 输出文件 | MSS.out + DSS.out | system.appimage |
| 烧录次数 | 2次 | 1次 |
| 开发效率 | 高（快速迭代） | 低（打包耗时） |
| 发布便利性 | 低（多文件） | 高（单文件） |
| 适用阶段 | 开发阶段 ✅ | 发布阶段 ✅ |

---

## 💡 建议

**开发阶段**（当前）：
- 保持分别编译MSS/DSS
- 快速调试，单独更新

**发布阶段**（将来）：
- 添加系统项目配置
- 生成.appimage发布

**最佳实践**：
- 项目同时保留两种方式
- 平时用分别编译，发布用系统打包
