/**
 * @file pose_model_wrapper.c
 * @brief AI 模型封装层实现
 * 
 * 支持两种后端:
 *   1. TI CNN 分类器库 (USE_TI_CNN_CLASSIFIER) - 推荐
 *   2. 模拟输出 (SIMULATE_MODEL) - 测试用
 * 
 * @version 2.0.0
 * @date 2025-12-09
 */

#include <stdint.h>
#include <string.h>
#include <math.h>

#include "pose_model_wrapper.h"

#ifdef USE_TI_CNN_CLASSIFIER
#include "cnn_classifier/cnn_classifier.h"
#endif

/*******************************************************************************
 * 内部变量
 ******************************************************************************/

/* 模型状态 */
static int32_t gModelReady = 0;

#ifdef USE_TI_CNN_CLASSIFIER
/* TI CNN 分类器句柄 */
static void *gCnnHandle = NULL;
#endif

/* 模型输入输出缓冲区 (静态分配，避免动态内存) */
static float gModelInput[MODEL_INPUT_SIZE];   /* 176 floats */
static float gModelOutput[MODEL_OUTPUT_SIZE]; /* 5 floats */

/* 统计信息 */
static uint32_t gTotalInferences = 0;
static uint32_t gTotalTimeUs = 0;

/* 类别名称 */
static const char* const CLASS_NAMES[MODEL_OUTPUT_SIZE] = {
    "Standing",
    "Sitting",
    "Lying",
    "Falling",
    "Walking"
};

/* 版本字符串 */
static const char VERSION_STRING[] = "2.0.0";

/*******************************************************************************
 * 初始化
 ******************************************************************************/

int32_t PoseModel_init(void)
{
    /* 清零缓冲区 */
    memset(gModelInput, 0, sizeof(gModelInput));
    memset(gModelOutput, 0, sizeof(gModelOutput));
    
    /* 重置统计 */
    gTotalInferences = 0;
    gTotalTimeUs = 0;
    
#ifdef USE_TI_CNN_CLASSIFIER
    /* 初始化 TI CNN 分类器 */
    {
        cnn_Classifier_moduleConfig config;
        int32_t errCode;
        
        config.num_frames = MODEL_NUM_FRAMES;       /* 8 */
        config.num_features = MODEL_NUM_FEATURES;   /* 22 */
        config.num_classes = MODEL_OUTPUT_SIZE;     /* 5 */
        config.scratchBuffer = NULL;  /* 内部分配 */
        config.scratchBufferSizeInBytes = 0;
        
        gCnnHandle = cnn_classifier_create(&config, &errCode);
        
        if (gCnnHandle == NULL || errCode != CNN_CLASSIFIER_EOK) {
            gModelReady = 0;
            return -1;
        }
    }
#endif
    
    gModelReady = 1;
    
    return 0;
}

/*******************************************************************************
 * 推理
 ******************************************************************************/

int32_t PoseModel_run(const float* input, float* output)
{
    /* 检查参数 */
    if (input == NULL || output == NULL) {
        return -1;
    }
    
    /* 检查模型状态 */
    if (!gModelReady) {
        return -2;
    }
    
    /* 复制输入数据到内部缓冲区 */
    memcpy(gModelInput, input, sizeof(gModelInput));
    
#ifdef USE_TI_CNN_CLASSIFIER
    /* 使用 TI CNN 分类器库 */
    if (gCnnHandle == NULL) {
        return -3;
    }
    
    cnn_classifier_predict(gCnnHandle, gModelInput, gModelOutput);
    
#elif defined(SIMULATE_MODEL)
    /* 模拟输出 (测试用) */
    {
        float maxVal = gModelInput[0];
        int32_t i;
        
        /* 简单规则: 根据输入特征模拟输出 */
        for (i = 1; i < MODEL_INPUT_SIZE; i++) {
            if (gModelInput[i] > maxVal) maxVal = gModelInput[i];
        }
        
        /* 设置默认概率分布 */
        gModelOutput[0] = 0.7f;  /* Standing */
        gModelOutput[1] = 0.1f;  /* Sitting */
        gModelOutput[2] = 0.1f;  /* Lying */
        gModelOutput[3] = 0.05f; /* Falling */
        gModelOutput[4] = 0.05f; /* Walking */
    }
#else
    #error "Must define USE_TI_CNN_CLASSIFIER or SIMULATE_MODEL"
#endif
    
    /* 复制输出到用户缓冲区 */
    memcpy(output, gModelOutput, sizeof(gModelOutput));
    
    /* 更新统计 */
    gTotalInferences++;
    
    return 0;
}

/*******************************************************************************
 * 分类结果
 ******************************************************************************/

int32_t PoseModel_getClass(const float* probs)
{
    int32_t maxIdx = 0;
    float maxProb;
    int32_t i;
    
    if (probs == NULL) {
        return -1;
    }
    
    maxProb = probs[0];
    
    for (i = 1; i < MODEL_OUTPUT_SIZE; i++) {
        if (probs[i] > maxProb) {
            maxProb = probs[i];
            maxIdx = i;
        }
    }
    
    return maxIdx;
}

int32_t PoseModel_getClassWithConfidence(
    const float* probs,
    int32_t* classIndex,
    float* confidence)
{
    int32_t maxIdx;
    
    if (probs == NULL || classIndex == NULL || confidence == NULL) {
        return -1;
    }
    
    maxIdx = PoseModel_getClass(probs);
    
    if (maxIdx < 0) {
        return -1;
    }
    
    *classIndex = maxIdx;
    *confidence = probs[maxIdx];
    
    return 0;
}

const char* PoseModel_getClassName(int32_t classIndex)
{
    if (classIndex < 0 || classIndex >= MODEL_OUTPUT_SIZE) {
        return "Unknown";
    }
    
    return CLASS_NAMES[classIndex];
}

/*******************************************************************************
 * 状态查询
 ******************************************************************************/

int32_t PoseModel_isReady(void)
{
    return gModelReady;
}

const char* PoseModel_getVersion(void)
{
    return VERSION_STRING;
}

void PoseModel_getStats(uint32_t* totalInferences, uint32_t* avgTimeUs)
{
    if (totalInferences != NULL) {
        *totalInferences = gTotalInferences;
    }
    
    if (avgTimeUs != NULL) {
        if (gTotalInferences > 0) {
            *avgTimeUs = gTotalTimeUs / gTotalInferences;
        } else {
            *avgTimeUs = 0;
        }
    }
}

void PoseModel_reset(void)
{
    memset(gModelInput, 0, sizeof(gModelInput));
    memset(gModelOutput, 0, sizeof(gModelOutput));
    gTotalInferences = 0;
    gTotalTimeUs = 0;
}

/*******************************************************************************
 * 辅助函数 (内部使用)
 ******************************************************************************/

/**
 * @brief 验证输出是否为有效概率分布
 * 
 * 检查:
 * 1. 所有值在 [0, 1] 范围内
 * 2. 所有值之和约等于 1.0
 * 
 * @param probs 概率数组
 * @return 1=有效, 0=无效
 */
static int32_t validateProbabilities(const float* probs)
{
    float sum = 0.0f;
    int32_t i;
    
    for (i = 0; i < MODEL_OUTPUT_SIZE; i++) {
        if (probs[i] < 0.0f || probs[i] > 1.0f) {
            return 0;
        }
        sum += probs[i];
    }
    
    /* Softmax 输出和应该接近 1.0 */
    if (fabsf(sum - 1.0f) > 0.01f) {
        return 0;
    }
    
    return 1;
}

/**
 * @brief 运行推理并验证输出
 * 
 * @param input  输入特征向量
 * @param output 输出概率向量
 * @return 0=成功且有效, 负数=错误
 */
int32_t PoseModel_runAndValidate(const float* input, float* output)
{
    int32_t ret;
    
    ret = PoseModel_run(input, output);
    
    if (ret != 0) {
        return ret;
    }
    
    if (!validateProbabilities(output)) {
        return -3; /* 输出无效 */
    }
    
    return 0;
}
