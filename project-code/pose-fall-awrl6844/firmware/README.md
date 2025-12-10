# 🔧 Firmware - AWRL6844 Pose and Fall Detection

**版本**: 2.0.0  
**更新日期**: 2025-12-09

---

## 📁 目录结构

```
firmware/
├── common/                       # 公共定义
│   ├── pose_types.h             # 类型和常量定义
│   ├── pose_config.h            # 平台配置参数
│   └── pose_ipc.h               # 双核 IPC 协议
│
├── mss/                          # MSS (R5F) 代码
│   ├── main.c                   # MSS 入口
│   ├── pose_mss.h               # MSS 模块头文件
│   ├── pose_mss.c               # MSS 模块实现
│   ├── mss_linker.cmd           # MSS 链接脚本
│   └── pose_mss_sdk_integration.c.example  # SDK 集成示例
│
├── dss/                          # DSS (C674x) 代码
│   ├── main.c                   # DSS 入口
│   ├── pose_dss.h               # DSS 模块头文件
│   ├── pose_dss.c               # DSS 模块实现
│   └── dss_linker.cmd           # DSS 链接脚本
│
├── model/                        # ML 模型
│   ├── cnn_classifier/          # TI CNN 分类器
│   │   ├── cnn_classifier.h     # API 接口
│   │   └── lib/                 # 预编译库 (R5F)
│   │       └── alg_cnnClassifier.xwrL684x.r5f.ti-arm-clang.release.lib
│   ├── pose_model_wrapper.h     # 模型封装层 v2.0
│   └── pose_model_wrapper.c     # 模型封装层实现
│
└── README.md                     # 本文件
```

---

## 🎯 架构说明

### 双核分工

| 核心 | 角色 | 主要任务 |
|------|------|----------|
| MSS (R5F) | 主控 | 配置、ML 推理、UART 输出 |
| DSS (C674x) | 协处理 | Range FFT、CFAR、点云生成 |

### 数据流

```
雷达采集 → DSS (信号处理) → IPC → MSS (特征提取 + ML 推理) → UART 输出
```

---

## 📊 ML 推理

### 模型参数

| 参数 | 值 |
|------|-----|
| 输入维度 | 176 (22 特征 × 8 帧) |
| 输出维度 | 5 类概率 |
| 后端 | TI CNN 分类器库 |

### 分类类别

| 索引 | 类别 | 中文 |
|------|------|------|
| 0 | Standing | 站立 |
| 1 | Sitting | 坐下 |
| 2 | Lying | 躺下 |
| 3 | Falling | 跌倒 |
| 4 | Walking | 行走 |

---

## 🔧 编译配置

### CCS 项目设置

**预定义符号**:
```
USE_TI_CNN_CLASSIFIER
```

**链接库**:
```
../model/cnn_classifier/lib/alg_cnnClassifier.xwrL684x.r5f.ti-arm-clang.release.lib
```

---

## 📚 API 参考

### 模型封装层

```c
// 初始化
int32_t PoseModel_init(void);

// 推理
int32_t PoseModel_run(const float* input, float* output);

// 获取分类结果
int32_t PoseModel_getClass(const float* probs);
const char* PoseModel_getClassName(int32_t classIndex);
```

---

**最后更新**: 2025-12-09
