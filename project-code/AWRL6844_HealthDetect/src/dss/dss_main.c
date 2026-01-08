/**
 * @file dss_main.c
 * @brief DSS侧主程序实现（C66x DSP）
 * 
 * 架构说明：
 * 这是标准mmw_demo DPC的扩展版本，增加了特征提取功能
 */

#include "dss_main.h"
#include "feature_extract.h"
#include <shared_memory.h>
#include <string.h>

/**************************************************************************
 * 全局变量
 **************************************************************************/

/* 特征数据指针（映射到共享RAM）*/
static HealthDetect_PointCloudFeatures_t *gFeatureData = NULL;

/**************************************************************************
 * 初始化
 **************************************************************************/

/**
 * @brief DSS侧初始化
 */
void DSS_HealthDetect_init(void)
{
    /* 映射共享RAM特征数据区 */
    gFeatureData = (HealthDetect_PointCloudFeatures_t *)FEATURE_DATA_BASE;
    
    /* TODO: 初始化标准DPC */
    /* TODO: 配置HWA */
    /* TODO: 配置EDMA */
}

/**************************************************************************
 * DPC执行（第3章 3.5.2节 数据流）
 **************************************************************************/

/**
 * @brief DSS侧DPC执行
 * 
 * 这是第3章架构的核心：在标准DPC后增加特征提取
 */
void DSS_HealthDetect_execute(void)
{
    /* Step 1: 从共享RAM读取DPC配置（MSS已写入）*/
    /* TODO: 读取配置 */
    
    /* Step 2: 执行标准DPC流程 */
    /* TODO: Range FFT (HWA) */
    /* TODO: Doppler FFT (HWA) */
    /* TODO: CFAR Detection (HWA + C66x) */
    /* TODO: AOA Processing (C66x) */
    /* 结果：得到点云数据（N个点，每个点有x/y/z/velocity） */
    
    /* 占位：假设已经得到点云 */
    void *pointCloud = NULL;  /* TODO: 指向实际点云数据 */
    void *sideInfo = NULL;    /* TODO: 指向SNR等信息 */
    uint32_t numPoints = 0;   /* TODO: 实际点数 */
    
    /* Step 3: 🔥特征提取（第3章核心创新）*/
    FeatureExtract_run(pointCloud, sideInfo, numPoints, gFeatureData);
    
    /* Step 4: 更新帧号 */
    static uint32_t frameNumber = 0;
    gFeatureData->frameNumber = frameNumber++;
    
    /* Step 5: 写入共享RAM（已自动写入，因为gFeatureData指向共享RAM）*/
    
    /* Step 6: 通知MSS完成 */
    /* TODO: 通过Mailbox通知MSS */
}

/**************************************************************************
 * 标准DPC辅助函数（占位，待实现）
 **************************************************************************/

/**
 * @brief 配置HWA执行Range FFT
 */
void DSS_configRangeFFT(void)
{
    /* TODO: 从mmw_demo学习HWA配置 */
}

/**
 * @brief 配置HWA执行Doppler FFT
 */
void DSS_configDopplerFFT(void)
{
    /* TODO: 从mmw_demo学习HWA配置 */
}

/**
 * @brief 执行CFAR检测
 */
void DSS_executeCFAR(void)
{
    /* TODO: 从mmw_demo学习CFAR算法 */
}

/**
 * @brief 执行AOA处理
 */
void DSS_executeAOA(void)
{
    /* TODO: 从mmw_demo学习AOA算法 */
}
