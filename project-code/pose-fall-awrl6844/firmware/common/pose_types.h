/**
 * @file pose_types.h
 * @brief Pose and Fall Detection 公共类型定义
 * 
 * 移植自 IWRL6432 Pose and Fall Detection Demo
 * 目标平台: AWRL6844EVM
 * 
 * @version 1.0.0
 * @date 2025-12-09
 */

#ifndef POSE_TYPES_H_
#define POSE_TYPES_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * 版本信息
 ******************************************************************************/
#define POSE_VERSION_MAJOR      1
#define POSE_VERSION_MINOR      0
#define POSE_VERSION_PATCH      0

/*******************************************************************************
 * 平台定义
 ******************************************************************************/
#define PLATFORM_AWRL6844       1       /* 目标平台: AWRL6844EVM */
#define PLATFORM_IWRL6432       0       /* 源平台: IWRL6432BOOST */

/*******************************************************************************
 * ML 分类参数 (来自 IWRL6432 pose.h)
 ******************************************************************************/

/* 时序帧数 - 8帧数据用于推理 */
#define NUM_FRAMES_OF_DATA      8

/* 分类阈值 */
#define NO_NN_RESULT            -1
#define CLASS_THRESHOLD         0.5f

/* TLV 消息类型 */
#define MMWDEMO_OUTPUT_MSG_ML_CLASSIFICATION_PROBABILITY    1031

/*******************************************************************************
 * 姿态分类定义
 ******************************************************************************/

/* 分类类型 ID */
#define ML_TYPE_WET_DRY             1
#define ML_TYPE_GRASS_NOTGRASS      2
#define ML_TYPE_HUMANPOSE           3       /* 人体姿态检测 */

/* 姿态分类数量 */
#define ML_TYPE_HUMANPOSE_CLASSES   5

/* 姿态分类索引 */
typedef enum {
    POSE_CLASS_STANDING = 0,    /* 站立 */
    POSE_CLASS_SITTING  = 1,    /* 坐下 */
    POSE_CLASS_LYING    = 2,    /* 躺下 */
    POSE_CLASS_FALLING  = 3,    /* 跌倒 */
    POSE_CLASS_WALKING  = 4,    /* 行走 */
    POSE_CLASS_COUNT    = 5
} PoseClassIndex_e;

/* 姿态分类名称 */
static const char* const POSE_CLASS_NAMES[POSE_CLASS_COUNT] = {
    "Standing",
    "Sitting", 
    "Lying",
    "Falling",
    "Walking"
};

/*******************************************************************************
 * 特征提取参数
 ******************************************************************************/

/* 每帧特征数量
 * 22个特征 = 7个运动特征 + 15个点云特征(5个点 × 3)
 * 运动特征: posz, velx, vely, velz, accx, accy, accz
 * 点云特征: (y, z, snr) × 5 个最高点
 */
#define ML_TYPE3_FEATURE_COUNT          22

/* 最少检测点数 */
#define ML_TYPE3_FEATURE_MIN_COUNT      5

/* 特征数据类型大小 (float32 = 4字节) */
#define ML_TYPE3_FEATURE_DATA_TYPE      4

/* 模型输入总维度 = 22特征 × 8帧 = 176 */
#define ML_INPUT_DIMENSION              (ML_TYPE3_FEATURE_COUNT * NUM_FRAMES_OF_DATA)

/* 模型输出维度 = 5类 */
#define ML_OUTPUT_DIMENSION             ML_TYPE_HUMANPOSE_CLASSES

/*******************************************************************************
 * Range Bin 配置 (用于特征提取)
 ******************************************************************************/

/* IWRL6432 原始配置 */
#define IWRL6432_NUM_RANGE_BINS         128
#define IWRL6432_FEATURE_START_BIN      6
#define IWRL6432_FEATURE_COUNT          25

/* AWRL6844 配置 (保持兼容) */
#define AWRL6844_NUM_RANGE_BINS         128     /* 128个采样点 = 128 range bins */
#define AWRL6844_FEATURE_START_BIN      6       /* 起始bin索引 */
#define AWRL6844_FEATURE_COUNT          25      /* 25个range bins用于AI */
#define AWRL6844_FEATURE_END_BIN        (AWRL6844_FEATURE_START_BIN + AWRL6844_FEATURE_COUNT - 1)

/*******************************************************************************
 * 数据结构定义
 ******************************************************************************/

/**
 * @brief ML 分类输出结构 (UART TLV 格式)
 */
typedef struct MmwDemo_output_message_ml_t
{
    uint32_t mlClassType;                           /**< 分类类型 ID */
    uint32_t mlClassCount;                          /**< 分类数量 */
    float    mlResult[ML_TYPE_HUMANPOSE_CLASSES];   /**< 各类别概率 (0-1) */
} MmwDemo_output_message_ml;

/**
 * @brief ML 属性配置
 */
typedef struct
{
    uint32_t   mlType;              /**< 分类类型 ID */
    uint32_t   mlClassNumber;       /**< 分类数量 */
    char       mlClass1[10];        /**< 类别1名称 */
    char       mlClass2[10];        /**< 类别2名称 */
    char       mlClass3[10];        /**< 类别3名称 */
    char       mlClass4[10];        /**< 类别4名称 */
    char       mlClass5[10];        /**< 类别5名称 */
} ML_Attrs;

/**
 * @brief 特征向量结构
 */
typedef struct
{
    float    data[ML_INPUT_DIMENSION];  /**< 176维输入特征 */
    uint32_t frameCount;                /**< 已收集的帧数 */
    uint8_t  isValid;                   /**< 特征是否有效 */
} PoseFeatureVector_t;

/**
 * @brief 分类结果结构
 */
typedef struct
{
    PoseClassIndex_e    classIndex;     /**< 分类索引 */
    float               confidence;     /**< 置信度 (0-1) */
    float               allProbs[POSE_CLASS_COUNT]; /**< 所有类别概率 */
    uint8_t             isValid;        /**< 结果是否有效 */
} PoseClassResult_t;

/**
 * @brief 点云点结构 (用于特征提取)
 */
typedef struct
{
    float x;        /**< X坐标 (米) */
    float y;        /**< Y坐标 (米) */
    float z;        /**< Z坐标 (米) */
    float velocity; /**< 速度 (米/秒) */
    float snr;      /**< 信噪比 (dB) */
} PointCloud_t;

/**
 * @brief 目标跟踪结构 (用于提取运动特征)
 */
typedef struct
{
    float posX;     /**< X位置 */
    float posY;     /**< Y位置 */
    float posZ;     /**< Z位置 */
    float velX;     /**< X速度 */
    float velY;     /**< Y速度 */
    float velZ;     /**< Z速度 */
    float accX;     /**< X加速度 */
    float accY;     /**< Y加速度 */
    float accZ;     /**< Z加速度 */
} TrackerTarget_t;

/*******************************************************************************
 * 错误码定义
 ******************************************************************************/
typedef enum
{
    POSE_OK                     = 0,
    POSE_ERROR_INVALID_PARAM    = -1,
    POSE_ERROR_NO_DATA          = -2,
    POSE_ERROR_INSUFFICIENT_POINTS = -3,
    POSE_ERROR_MODEL_FAILED     = -4,
    POSE_ERROR_NOT_READY        = -5,
    POSE_ERROR_TIMEOUT          = -6
} PoseError_e;

#ifdef __cplusplus
}
#endif

#endif /* POSE_TYPES_H_ */
