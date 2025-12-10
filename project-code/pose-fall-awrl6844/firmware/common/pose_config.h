/**
 * @file pose_config.h
 * @brief Pose and Fall Detection 平台配置
 * 
 * 平台特定的配置参数
 * 
 * @version 1.0.0
 * @date 2025-12-09
 */

#ifndef POSE_CONFIG_H_
#define POSE_CONFIG_H_

#include "pose_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * AWRL6844 硬件配置
 ******************************************************************************/

/* 天线配置 */
#define AWRL6844_NUM_TX_ANTENNAS        4       /* 发射天线数 */
#define AWRL6844_NUM_RX_ANTENNAS        4       /* 接收天线数 */
#define AWRL6844_NUM_VIRTUAL_ANTENNAS   16      /* 虚拟天线数 (4×4) */

/* 当前使用的天线配置 (保守模式) */
#define CFG_NUM_TX_ANTENNAS             2       /* 使用2个TX */
#define CFG_NUM_RX_ANTENNAS             3       /* 使用3个RX */
#define CFG_NUM_VIRTUAL_ANTENNAS        6       /* 6个虚拟天线 */

/* CPU 配置 */
#define MSS_CLOCK_MHZ                   200     /* R4F 主频 */
#define DSS_CLOCK_MHZ                   600     /* C674x DSP 主频 */

/*******************************************************************************
 * 雷达配置参数 (来自 awrl6844_pose_detection_v1_conservative.cfg)
 ******************************************************************************/

/* 基础配置 */
#define CFG_START_FREQ_GHZ              60.25f  /* 起始频率 */
#define CFG_IDLE_TIME_US                7.0f    /* 空闲时间 */
#define CFG_ADC_START_TIME_US           4.5f    /* ADC 起始时间 */
#define CFG_RAMP_END_TIME_US            53.0f   /* 斜坡结束时间 */
#define CFG_FREQ_SLOPE_MHZ_US           29.982f /* 频率斜率 */

/* 帧配置 */
#define CFG_NUM_ADC_SAMPLES             128     /* ADC 采样点数 */
#define CFG_NUM_CHIRPS_PER_FRAME        8       /* 每帧chirp数 (2TX × 4loops) */
#define CFG_FRAME_PERIODICITY_MS        100     /* 帧周期 (10 fps) */

/* Range 配置 */
#define CFG_NUM_RANGE_BINS              128     /* Range bins 数量 */
#define CFG_RANGE_RESOLUTION_M          0.0385f /* 距离分辨率 (米) */
#define CFG_MAX_RANGE_M                 4.928f  /* 最大检测距离 (米) */

/* Doppler 配置 */
#define CFG_NUM_DOPPLER_BINS            8       /* Doppler bins */
#define CFG_VELOCITY_RESOLUTION_MPS     0.05f   /* 速度分辨率 (米/秒) */
#define CFG_MAX_VELOCITY_MPS            0.2f    /* 最大速度 (米/秒) */

/*******************************************************************************
 * 数据处理配置
 ******************************************************************************/

/* FFT 配置 */
#define RANGE_FFT_SIZE                  128     /* Range FFT 大小 */
#define DOPPLER_FFT_SIZE                8       /* Doppler FFT 大小 */

/* CFAR 配置 */
#define CFAR_GUARD_LEN                  4       /* 保护单元长度 */
#define CFAR_NOISE_LEN                  8       /* 噪声单元长度 */
#define CFAR_THRESHOLD_DB               12.0f   /* 检测阈值 (dB) */

/* 点云配置 */
#define MAX_NUM_DETECTED_POINTS         256     /* 最大检测点数 */
#define MIN_NUM_DETECTED_POINTS         5       /* 最小检测点数 (用于ML) */

/* 跟踪器配置 */
#define MAX_NUM_TRACKS                  10      /* 最大跟踪目标数 */

/*******************************************************************************
 * ML 推理配置
 ******************************************************************************/

/* 推理控制 */
#define ML_INFERENCE_ENABLED            1       /* 启用ML推理 */
#define ML_MIN_FRAMES_BEFORE_INFERENCE  8       /* 推理前最少帧数 */
#define ML_INFERENCE_INTERVAL_FRAMES    1       /* 推理间隔帧数 */

/* 特征提取 */
#define ML_FEATURE_EXTRACT_ENABLED      1       /* 启用特征提取 */
#define ML_MAX_HEIGHT_POINTS            5       /* 最高点数量 (用于特征) */

/* 输出控制 */
#define ML_UART_OUTPUT_ENABLED          1       /* UART输出ML结果 */
#define ML_DEBUG_PRINT_ENABLED          0       /* 调试打印 (生产环境关闭) */

/*******************************************************************************
 * UART 配置
 ******************************************************************************/

#define UART_CLI_BAUD_RATE              115200  /* CLI 串口波特率 */
#define UART_DATA_BAUD_RATE             921600  /* 数据串口波特率 */

/*******************************************************************************
 * 内存配置
 ******************************************************************************/

/* L3 内存分配 */
#define L3_RAM_SIZE_KB                  256     /* L3 RAM 大小 */
#define RADAR_CUBE_SIZE_KB              64      /* 雷达数据立方体 */
#define DETECTION_MATRIX_SIZE_KB        16      /* 检测矩阵 */

/* 堆栈大小 */
#define MSS_MAIN_TASK_STACK_SIZE        4096    /* MSS 主任务堆栈 */
#define MSS_INIT_TASK_STACK_SIZE        2048    /* MSS 初始化任务堆栈 */
#define ML_TASK_STACK_SIZE              4096    /* ML 任务堆栈 */

/*******************************************************************************
 * 性能监控配置
 ******************************************************************************/

#define PERF_BENCHMARK_ENABLED          0       /* 性能基准测试 */
#define PERF_PROFILER_ENABLED           0       /* 性能分析器 */

#define FRAME_REF_TIMER_CLOCK_MHZ       40      /* 帧计时器时钟 */

#ifdef __cplusplus
}
#endif

#endif /* POSE_CONFIG_H_ */
