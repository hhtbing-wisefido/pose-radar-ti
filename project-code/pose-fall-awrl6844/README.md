# 🎯 AWRL6844 Pose and Fall Detection

**更新日期**: 2025-12-10  
**状态**: ✅ FreeRTOS 固件编译成功

---

## 📁 目录结构

```
pose-fall-awrl6844/
├── firmware/              # 固件源码
│   ├── mss/              # MSS (R5F) - FreeRTOS
│   │   ├── main.c        # ✅ FreeRTOS 主入口
│   │   ├── pose_mss.c    # ✅ SDK 6.x DPU API
│   │   ├── gtrack_alloc.c # ✅ GTRACK 内存分配
│   │   └── generated/    # ✅ SysConfig 生成
│   ├── model/            # AI 模型封装
│   └── common/           # 公共头文件
├── build/                # ✅ 构建输出
│   └── pose_fall_awrl6844.out  # ✅ 固件镜像 (860 KB)
├── tools/                # 构建脚本
│   ├── compile_syscfg.bat
│   └── link.bat
└── build_all.bat         # ✅ 一键构建
```

---

## 🚀 进度

| Phase | 状态 | 说明 |
|-------|------|------|
| Phase 1-3 | ✅ | 数据处理、模型移植 |
| Phase 4 | ✅ | TI CNN 分类器集成 |
| **Phase 5** | **✅** | **FreeRTOS 固件编译成功** |
| Phase 6 | ⏳ | 硬件烧录测试 |

---

## 🛠️ 快速构建

### 一键构建（推荐）

```bash
build_all.bat
```

### 分步构建

```bash
# 1. SysConfig 生成配置
tools\compile_syscfg.bat

# 2. 编译源文件
# (参见 tools\compile_syscfg.bat)

# 3. 链接固件
tools\link.bat
```

---

## 🔧 技术栈

- **SDK**: mmWave SDK 6.01.00.01
- **RTOS**: FreeRTOS (SDK 内置)
- **编译器**: TI ARM Clang 4.0.4 LTS
- **平台**: AWRL6844 (Cortex-R5F)
- **AI**: TI CNN 分类器库

---

## 📦 编译输出

| 文件 | 大小 | 格式 | 用途 |
|------|------|------|------|
| `pose_fall_awrl6844.out` | 860 KB | ELF | 调试、分析 |
| `pose_fall_awrl6844_r5.rig` | 88 KB | RIG | 核心镜像 |
| **`pose_fall_awrl6844.appimage`** | **100 KB** | **APPIMAGE** | **✅ 烧录到硬件** |

### 固件格式说明

AWRL6844 的 ROM 引导加载程序（RBL）需要 `.appimage` 格式：

```
.out (ELF) → HEX → .rig (Core) → .appimage (Boot)
```

**生成命令**: `tools\gen_appimage.bat`

---

## ⏭️ 下一步

1. **硬件烧录**: 使用 Uniflash 工具烧录固件
2. **串口监控**: 配置 115200 波特率查看日志
3. **功能测试**: 验证雷达、DPU、AI 推理功能
4. **性能优化**: 分析内存和 CPU 使用率
