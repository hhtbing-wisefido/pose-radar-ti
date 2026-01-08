# 📁 DSS Layer - DSP算法层

## 🎯 层职责

DSS (DSP Subsystem) 层运行在 C66x DSP 核心上，负责：

- 🔄 **信号处理** - Range FFT, Doppler FFT
- 🎯 **目标检测** - CFAR检测，AOA估计
- 📊 **点云生成** - 笛卡尔坐标转换
- 🧠 **特征提取** - 健康检测特征（🆕新增功能）

## 📂 文件列表

| 文件 | 描述 |
|------|------|
| `dss_main.h` | DSS主程序头文件 |
| `dss_main.c` | DSS主程序实现，IPC处理 |
| `feature_extract.h` | 特征提取模块头文件 |
| `feature_extract.c` | 特征提取实现（🆕新增） |
| `dsp_utils.h` | DSP工具函数头文件 |
| `dsp_utils.c` | DSP工具函数实现 |

## ⚙️ 运行模式

DSS运行在C66x DSP核心，**不使用FreeRTOS**：
- 裸机运行或最小化RTOS
- IPC通知驱动处理
- 使用SDK DPL层进行同步

```c
// DSS主循环
void DSS_main(void)
{
    DSS_init();
    
    while (1)
    {
        // 等待MSS的IPC通知
        if (gIpcNotifyFlag)
        {
            gIpcNotifyFlag = 0;
            DSS_processFrame();
        }
    }
}
```

## 🔗 依赖关系

```
DSS Layer
    ├── common/           # 共享头文件
    ├── SDK DPL           # 驱动移植层
    ├── TI DSPLIB         # DSP优化库（可选）
    └── IPC               # 多核通信
```

## 📝 编译器

- **编译器**: TI C6000 8.5.0.LTS
- **目标**: C66x DSP
- **选项**: `-mv6600 --abi=eabi`

## 🆕 新增功能：特征提取

Health Detect项目新增的功能，在DSS上运行：

```c
// feature_extract.c
typedef struct HealthDetect_Features
{
    StatisticsInfo_t rangeStats;      // 距离统计
    StatisticsInfo_t velocityStats;   // 速度统计
    float motionEnergy;               // 运动能量
    float motionEnergySmoothed;       // 平滑运动能量
    float peakSnr_dB;                 // 峰值信噪比
    uint16_t numValidPoints;          // 有效点数
} HealthDetect_Features_t;
```

---

> 🔴 **注意**: DSS代码需要针对C66x DSP优化，生产环境建议使用TI DSPLIB。
