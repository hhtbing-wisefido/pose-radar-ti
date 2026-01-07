/**
 * @file feature_extract.c
 * @brief 点云特征提取实现（运行在C66x DSP）
 * 
 * 参考：第3章 3.3.2节 算法部署决策
 */

#include "feature_extract.h"
#include "common/shared_memory.h"
#include <string.h>
#include <math.h>

/* SDK点云数据格式（从mmw_demo学习） */
typedef struct {
    float x;
    float y;
    float z;
    float velocity;
} PointCloudCartesian_t;

/**************************************************************************
 * 特征提取主函数
 **************************************************************************/
void FeatureExtract_run(
    const void *pointCloud,
    const void *sideInfo,
    uint32_t numPoints,
    HealthDetect_PointCloudFeatures_t *features)
{
    /* 清零输出 */
    memset(features, 0, sizeof(HealthDetect_PointCloudFeatures_t));
    
    features->numPoints = numPoints;
    
    /* 如果没有点云，直接返回 */
    if (numPoints == 0) {
        return;
    }
    
    /* 1. 计算质心 */
    FeatureExtract_computeCenter(
        pointCloud, numPoints,
        &features->centerX,
        &features->centerY,
        &features->centerZ
    );
    
    /* 2. 计算分布范围 */
    FeatureExtract_computeSpread(
        pointCloud, numPoints,
        features->centerX,
        features->centerY,
        features->centerZ,
        &features->spreadXY,
        &features->spreadZ
    );
    
    /* 3. 计算速度特征 */
    FeatureExtract_computeVelocity(
        pointCloud, numPoints,
        &features->avgVelocity,
        &features->maxVelocity
    );
}

/**************************************************************************
 * 质心计算
 **************************************************************************/
void FeatureExtract_computeCenter(
    const void *pointCloud,
    uint32_t numPoints,
    float *centerX,
    float *centerY,
    float *centerZ)
{
    const PointCloudCartesian_t *points = (const PointCloudCartesian_t *)pointCloud;
    float sumX = 0.0f, sumY = 0.0f, sumZ = 0.0f;
    uint32_t i;
    
    /* 累加所有点的坐标 */
    for (i = 0; i < numPoints; i++) {
        sumX += points[i].x;
        sumY += points[i].y;
        sumZ += points[i].z;
    }
    
    /* 计算平均值（质心） */
    *centerX = sumX / (float)numPoints;
    *centerY = sumY / (float)numPoints;
    *centerZ = sumZ / (float)numPoints;
}

/**************************************************************************
 * 分布范围计算
 **************************************************************************/
void FeatureExtract_computeSpread(
    const void *pointCloud,
    uint32_t numPoints,
    float centerX,
    float centerY,
    float centerZ,
    float *spreadXY,
    float *spreadZ)
{
    const PointCloudCartesian_t *points = (const PointCloudCartesian_t *)pointCloud;
    float sumXY = 0.0f, sumZ = 0.0f;
    float dx, dy, dz;
    uint32_t i;
    
    /* 计算到质心的距离平方和 */
    for (i = 0; i < numPoints; i++) {
        dx = points[i].x - centerX;
        dy = points[i].y - centerY;
        dz = points[i].z - centerZ;
        
        sumXY += (dx * dx + dy * dy);
        sumZ += (dz * dz);
    }
    
    /* 计算RMS（均方根）作为分布范围 */
    *spreadXY = sqrtf(sumXY / (float)numPoints);
    *spreadZ = sqrtf(sumZ / (float)numPoints);
}

/**************************************************************************
 * 速度特征计算
 **************************************************************************/
void FeatureExtract_computeVelocity(
    const void *pointCloud,
    uint32_t numPoints,
    float *avgVelocity,
    float *maxVelocity)
{
    const PointCloudCartesian_t *points = (const PointCloudCartesian_t *)pointCloud;
    float sumVelocity = 0.0f;
    float maxVel = 0.0f;
    float absVel;
    uint32_t i;
    
    for (i = 0; i < numPoints; i++) {
        absVel = fabsf(points[i].velocity);
        sumVelocity += absVel;
        
        if (absVel > maxVel) {
            maxVel = absVel;
        }
    }
    
    *avgVelocity = sumVelocity / (float)numPoints;
    *maxVelocity = maxVel;
}
