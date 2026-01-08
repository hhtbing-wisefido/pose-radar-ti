/**
 * @file feature_extract.h
 * @brief 点云特征提取模块（运行在C66x DSP）
 * 
 * 参考：第3章 3.3.2节 算法部署决策
 * 这是第3章架构的核心创新：利用C66x DSP的450MHz算力进行浮点密集运算
 */

#ifndef FEATURE_EXTRACT_H
#define FEATURE_EXTRACT_H

#include <stdint.h>
#include <health_detect_types.h>

/**************************************************************************
 * 特征提取主函数
 **************************************************************************/

/**
 * @brief 从点云数据提取特征（在DSS/C66x执行）
 * 
 * @param pointCloud  输入：点云数据（从HWA输出，含x/y/z/velocity）
 * @param sideInfo    输入：点云附加信息（SNR等）
 * @param numPoints   输入：点云数量
 * @param features    输出：特征数据（写入共享RAM）
 * 
 * 功能：
 * 1. 计算质心 (centerX/Y/Z)
 * 2. 计算分布范围 (spreadXY/Z)
 * 3. 计算速度特征 (avgVelocity/maxVelocity)
 * 4. 统计点数 (numPoints)
 * 
 * 部署位置：DSS/C66x @ 450MHz
 * 原因：浮点累加/除法密集运算，DSP优势明显
 */
void FeatureExtract_run(
    const void *pointCloud,         /* SDK点云格式 */
    const void *sideInfo,           /* SDK sideInfo格式 */
    uint32_t numPoints,
    HealthDetect_PointCloudFeatures_t *features
);

/**************************************************************************
 * 子功能函数
 **************************************************************************/

/**
 * @brief 计算点云质心坐标
 * 
 * 算法：centerX = Σx / N, centerY = Σy / N, centerZ = Σz / N
 * 复杂度：O(N) 浮点累加 + 3次浮点除法
 */
void FeatureExtract_computeCenter(
    const void *pointCloud,
    uint32_t numPoints,
    float *centerX,
    float *centerY,
    float *centerZ
);

/**
 * @brief 计算点云分布范围
 * 
 * 算法：spreadXY = sqrt(Σ(x²+y²) / N), spreadZ = sqrt(Σz² / N)
 * 复杂度：O(N) 浮点运算 + 2次sqrt
 */
void FeatureExtract_computeSpread(
    const void *pointCloud,
    uint32_t numPoints,
    float centerX,
    float centerY,
    float centerZ,
    float *spreadXY,
    float *spreadZ
);

/**
 * @brief 计算速度特征
 * 
 * 算法：avgVelocity = Σv / N, maxVelocity = max(|v|)
 * 复杂度：O(N) 浮点累加 + 比较
 */
void FeatureExtract_computeVelocity(
    const void *pointCloud,
    uint32_t numPoints,
    float *avgVelocity,
    float *maxVelocity
);

#endif /* FEATURE_EXTRACT_H */
