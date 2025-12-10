/**
 * @file pose_mss.h
 * @brief MSS (Master Subsystem) 姿态检测模块头文件
 * 
 * 运行在 ARM Cortex-R4F 上的主控代码
 * 负责: 系统初始化、CLI处理、ML推理、UART输出
 * 
 * @version 1.0.0
 * @date 2025-12-09
 */

#ifndef POSE_MSS_H_
#define POSE_MSS_H_

#include "../common/pose_types.h"
#include "../common/pose_config.h"
#include "../common/pose_ipc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * MSS 状态定义
 ******************************************************************************/

typedef enum {
    MSS_STATE_INIT = 0,         /**< 初始化状态 */
    MSS_STATE_IDLE,             /**< 空闲状态 */
    MSS_STATE_RUNNING,          /**< 运行状态 */
    MSS_STATE_STOPPED,          /**< 停止状态 */
    MSS_STATE_ERROR             /**< 错误状态 */
} MssState_e;

/*******************************************************************************
 * MSS 控制块结构
 ******************************************************************************/

/**
 * @brief MSS 主控制块
 * 
 * 包含所有MSS运行时状态和配置
 */
typedef struct MmwDemo_MSS_MCB_t
{
    /* 系统状态 */
    MssState_e              state;              /**< 当前状态 */
    uint8_t                 isRunning;          /**< 是否正在运行 */
    uint8_t                 sensorStarted;      /**< 传感器是否已启动 */
    
    /* 帧计数 */
    uint32_t                frameCount;         /**< 帧计数 */
    uint32_t                subFrameCount;      /**< 子帧计数 */
    
    /* ML 相关 */
    PoseFeatureVector_t     featureVector;      /**< 特征向量 */
    PoseClassResult_t       classResult;        /**< 分类结果 */
    uint8_t                 mlReady;            /**< ML是否就绪 */
    
    /* 点云数据 */
    PointCloud_t*           pointCloud;         /**< 点云数据指针 */
    uint32_t                numDetectedPoints;  /**< 检测点数量 */
    
    /* 跟踪目标 */
    TrackerTarget_t*        trackerTargets;     /**< 跟踪目标指针 */
    uint32_t                numTrackedTargets;  /**< 跟踪目标数量 */
    
    /* 配置 */
    ML_Attrs                mlAttrs;            /**< ML属性配置 */
    
    /* 统计信息 */
    uint32_t                mlInferenceCount;   /**< ML推理次数 */
    uint32_t                mlErrorCount;       /**< ML错误次数 */
    float                   avgInferenceTimeUs; /**< 平均推理时间 */
    
} MmwDemo_MSS_MCB;

/*******************************************************************************
 * 全局变量声明
 ******************************************************************************/

extern MmwDemo_MSS_MCB gMmwMssMCB;

/*******************************************************************************
 * MSS 初始化函数
 ******************************************************************************/

/**
 * @brief 初始化 MSS 模块
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_init(void);

/**
 * @brief 初始化 ML 模块
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_initML(void);

/**
 * @brief 初始化数据处理链
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_initDataPath(void);

/*******************************************************************************
 * MSS 运行时函数
 ******************************************************************************/

/**
 * @brief 启动传感器
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_sensorStart(void);

/**
 * @brief 停止传感器
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_sensorStop(void);

/**
 * @brief 处理帧数据
 * @param[in] frameNum 帧编号
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_processFrame(uint32_t frameNum);

/*******************************************************************************
 * 特征提取函数
 ******************************************************************************/

/**
 * @brief 从点云提取特征
 * @param[in] points 点云数据
 * @param[in] numPoints 点数量
 * @param[in] target 跟踪目标
 * @param[out] features 输出特征数组 (22个float)
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_extractFeatures(
    const PointCloud_t* points,
    uint32_t numPoints,
    const TrackerTarget_t* target,
    float* features
);

/**
 * @brief 更新特征向量缓冲区
 * @param[in] features 当前帧特征
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_updateFeatureBuffer(const float* features);

/**
 * @brief 选择最高的N个点 (用于特征提取)
 * @param[in] points 输入点云
 * @param[in] numPoints 点数量
 * @param[out] selectedPoints 选中的点
 * @param[in] maxSelect 最大选择数量
 * @return 实际选择的点数
 */
uint32_t MSS_selectMaxHeightPoints(
    const PointCloud_t* points,
    uint32_t numPoints,
    PointCloud_t* selectedPoints,
    uint32_t maxSelect
);

/*******************************************************************************
 * ML 推理函数
 ******************************************************************************/

/**
 * @brief 执行 ML 推理
 * @param[out] result 分类结果
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_runInference(PoseClassResult_t* result);

/**
 * @brief 检查是否可以执行推理
 * @return 1 可以, 0 不可以
 */
uint8_t MSS_canRunInference(void);

/*******************************************************************************
 * UART 输出函数
 ******************************************************************************/

/**
 * @brief 发送 ML 结果到 UART
 * @param[in] result 分类结果
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_sendMLResult(const PoseClassResult_t* result);

/**
 * @brief 发送帧头信息
 * @param[in] frameNum 帧编号
 * @param[in] numPoints 检测点数
 * @return POSE_OK 成功, 其他错误码
 */
PoseError_e MSS_sendFrameHeader(uint32_t frameNum, uint32_t numPoints);

/*******************************************************************************
 * 调试函数
 ******************************************************************************/

#if ML_DEBUG_PRINT_ENABLED
/**
 * @brief 打印特征向量
 * @param[in] features 特征数组
 * @param[in] numFeatures 特征数量
 */
void MSS_debugPrintFeatures(const float* features, uint32_t numFeatures);

/**
 * @brief 打印分类结果
 * @param[in] result 分类结果
 */
void MSS_debugPrintResult(const PoseClassResult_t* result);
#endif

#ifdef __cplusplus
}
#endif

#endif /* POSE_MSS_H_ */
