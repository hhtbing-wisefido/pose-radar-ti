/**
 * @file pose_model_wrapper.h
 * @brief AI 模型封装层接口
 * 
 * 支持两种后端:
 *   - USE_TI_CNN_CLASSIFIER: 使用 TI CNN 分类器库 (推荐)
 *   - SIMULATE_MODEL: 模拟输出 (测试用)
 * 
 * @version 2.0.0
 * @date 2025-12-09
 */

#ifndef POSE_MODEL_WRAPPER_H_
#define POSE_MODEL_WRAPPER_H_

#include <stdint.h>
#include "../common/pose_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * 模型信息
 ******************************************************************************/

/* 模型版本 */
#define POSE_MODEL_VERSION_MAJOR    2
#define POSE_MODEL_VERSION_MINOR    0
#define POSE_MODEL_VERSION_PATCH    0

/* 模型参数 */
#define MODEL_NUM_FRAMES            8       /* 帧数 */
#define MODEL_NUM_FEATURES          22      /* 每帧特征数 */
#define MODEL_INPUT_SIZE            176     /* 22 features × 8 frames */
#define MODEL_OUTPUT_SIZE           5       /* 5 classes */

/* 目标平台 */
#define MODEL_TARGET_PLATFORM       "AWRL6844"
#define MODEL_TARGET_CPU            "Cortex-R4F"

/*******************************************************************************
 * 函数声明
 ******************************************************************************/

/**
 * @brief 初始化模型
 * 
 * 分配必要的缓冲区，准备模型推理
 * 
 * @return 0=成功, 其他=错误码
 */
int32_t PoseModel_init(void);

/**
 * @brief 运行 ML 推理
 * 
 * @param input  输入特征向量 (176 floats = 22特征 × 8帧)
 * @param output 输出概率向量 (5 floats = 5类概率)
 * @return 0=成功, 其他=错误码
 * 
 * @note 输入数据格式:
 *       - 每帧 22 个特征: [posz, velx, vely, velz, accx, accy, accz, 
 *                         y1, z1, snr1, y2, z2, snr2, ..., y5, z5, snr5]
 *       - 8 帧数据按时间顺序排列
 *       - 总计 176 个 float32 值
 * 
 * @note 输出数据格式:
 *       - 5 个概率值，对应 5 个姿态类别
 *       - [Standing, Sitting, Lying, Falling, Walking]
 *       - 所有概率之和 ≈ 1.0 (Softmax 输出)
 */
int32_t PoseModel_run(const float* input, float* output);

/**
 * @brief 获取分类结果
 * 
 * 从概率数组中找出最大概率对应的类别
 * 
 * @param probs 概率数组 (5 floats)
 * @return 最大概率的类别索引 (0-4)
 */
int32_t PoseModel_getClass(const float* probs);

/**
 * @brief 获取分类结果 (带置信度)
 * 
 * @param probs      概率数组 (5 floats)
 * @param classIndex [out] 分类索引
 * @param confidence [out] 置信度 (0.0 - 1.0)
 * @return 0=成功
 */
int32_t PoseModel_getClassWithConfidence(
    const float* probs,
    int32_t* classIndex,
    float* confidence
);

/**
 * @brief 获取类别名称
 * 
 * @param classIndex 类别索引 (0-4)
 * @return 类别名称字符串，无效索引返回 "Unknown"
 */
const char* PoseModel_getClassName(int32_t classIndex);

/**
 * @brief 检查模型是否就绪
 * 
 * @return 1=就绪, 0=未就绪
 */
int32_t PoseModel_isReady(void);

/**
 * @brief 获取模型版本字符串
 * 
 * @return 版本字符串 (如 "1.0.0")
 */
const char* PoseModel_getVersion(void);

/**
 * @brief 获取推理统计信息
 * 
 * @param totalInferences [out] 总推理次数
 * @param avgTimeUs       [out] 平均推理时间 (微秒)
 */
void PoseModel_getStats(uint32_t* totalInferences, uint32_t* avgTimeUs);

/**
 * @brief 重置模型状态
 * 
 * 清除内部缓冲区和统计信息
 */
void PoseModel_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* POSE_MODEL_WRAPPER_H_ */
