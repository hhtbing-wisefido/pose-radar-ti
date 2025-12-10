/**
 * @file pose_mss.c
 * @brief MSS (Master Subsystem) 姿态检测模块实现
 * 
 * 移植自 IWRL6432 Pose and Fall Detection Demo (pose.c)
 * 目标平台: AWRL6844EVM (ARM Cortex-R4F)
 * 
 * @version 1.0.0
 * @date 2025-12-09
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "pose_mss.h"

/* mmWave SDK 6.x - 按正确顺序 include */

/* 1. DPL (Driver Porting Layer) */
#include <kernel/dpl/DebugP.h>
#include <kernel/dpl/SemaphoreP.h>
#include <kernel/dpl/CacheP.h>

/* 2. FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* 3. 驱动 (必须在 datapath 之前) */
#include <drivers/hw_include/cslr_soc.h>
#include <drivers/edma.h>
#include <drivers/uart.h>
#include <drivers/ipc_notify.h>

/* 4. 数据路径 */
#include <datapath/dpedma/v1/dpedma.h>
#include <datapath/dpu/rangeproc/v1/rangeprochwa.h>
#include <datapath/dpu/cfarproc/v1/cfarprochwa.h>
#include <datapath/dpu/dopplerproc/v1/dopplerprochwa.h>
#include <datapath/dpu/trackerproc/v0/trackerproc.h>

/* 5. 控制模块 */
#include <control/mmwave/mmwave.h>

/* 6. 工具 */
#include <utils/mathutils/mathutils.h>

/*******************************************************************************
 * 全局变量
 ******************************************************************************/

/* MSS 主控制块 */
MmwDemo_MSS_MCB gMmwMssMCB = {0};

/* DPU 句柄 */
static DPU_RangeProcHWA_Handle gRangeProcHandle = NULL;
static DPU_CFARProcHWA_Handle gCfarProcHandle = NULL;
static DPU_TrackerProc_Handle gTrackerHandle = NULL;
static MMWave_Handle gMmwaveHandle = NULL;

/* 共享内存缓冲区 (L3 RAM) */
#pragma DATA_SECTION(gRadarCube, ".radarCube")
static uint16_t gRadarCube[CFG_NUM_VIRTUAL_ANTENNAS][CFG_NUM_CHIRPS_PER_FRAME][CFG_NUM_RANGE_BINS];

#pragma DATA_SECTION(gDetectionMatrix, ".detectionMatrix")
static uint16_t gDetectionMatrix[CFG_NUM_VIRTUAL_ANTENNAS][CFG_NUM_RANGE_BINS];

#pragma DATA_SECTION(gPointCloudBuf, ".pointCloud")
static PointCloud_t gPointCloudBuf[MAX_NUM_DETECTED_POINTS];

/* CFAR 检测结果列表 (SDK 6.x) */
#pragma DATA_SECTION(gCfarDetList, ".cfarDetList")
DPIF_CFARDetList gCfarDetList[MAX_NUM_DETECTED_POINTS];

/* 点云扩展格式 (用于 Tracker) */
#pragma DATA_SECTION(gPointCloudExt, ".pointCloudExt")
static DPIF_PointCloudCartesianExt gPointCloudExt[MAX_NUM_DETECTED_POINTS];

#pragma DATA_SECTION(gTrackerTargetsBuf, ".trackerTargets")
static TrackerTarget_t gTrackerTargetsBuf[MAX_NUM_TRACKS];

/* 特征向量临时缓冲区 (循环缓冲) */
static int32_t gInputVector[ML_TYPE3_FEATURE_COUNT * NUM_FRAMES_OF_DATA] = {0};
static int32_t gInputVectorTemp[ML_TYPE3_FEATURE_COUNT * NUM_FRAMES_OF_DATA] = {0};

/* 模型输入输出缓冲区 */
static float gModelInput[NUM_FRAMES_OF_DATA * ML_TYPE3_FEATURE_COUNT] = {0.0f};
static float gModelOutput[ML_TYPE_HUMANPOSE_CLASSES] = {-1.0f};

/* 全局数据集 (从点云/跟踪器提取的当前帧特征) */
float gDataSet[ML_TYPE3_FEATURE_COUNT] = {0.0f};

/* ML 属性 */
ML_Attrs gMLAttrs = {
    .mlType        = ML_TYPE_HUMANPOSE,
    .mlClassNumber = ML_TYPE_HUMANPOSE_CLASSES,
    .mlClass1      = "Stood",
    .mlClass2      = "Sat",
    .mlClass3      = "Lying",
    .mlClass4      = "Falling",
    .mlClass5      = "Walking",
};

/* 分类结果 */
MmwDemo_output_message_ml gClassificationResult = {0};

/*******************************************************************************
 * 内部函数声明
 ******************************************************************************/
static void CreateFeatureVector(void);
static int32_t RunTVMModel(float* outputBuf);

/*******************************************************************************
 * MSS 初始化
 ******************************************************************************/

PoseError_e MSS_init(void)
{
    /* 初始化控制块 */
    memset(&gMmwMssMCB, 0, sizeof(MmwDemo_MSS_MCB));
    
    gMmwMssMCB.state = MSS_STATE_INIT;
    gMmwMssMCB.isRunning = 0;
    gMmwMssMCB.sensorStarted = 0;
    gMmwMssMCB.frameCount = 0;
    
    /* 复制 ML 属性 */
    memcpy(&gMmwMssMCB.mlAttrs, &gMLAttrs, sizeof(ML_Attrs));
    
    /* 初始化特征向量 */
    gMmwMssMCB.featureVector.frameCount = 0;
    gMmwMssMCB.featureVector.isValid = 0;
    
    /* 初始化分类结果 */
    gMmwMssMCB.classResult.isValid = 0;
    
    /* 初始化 ML */
    if (MSS_initML() != POSE_OK) {
        gMmwMssMCB.state = MSS_STATE_ERROR;
        return POSE_ERROR_MODEL_FAILED;
    }
    
    /* 初始化 IPC (Phase 3.3) - SDK 6.x 使用 IPC_Notify */
    /* IPC 初始化由 SysConfig 生成的代码完成
     * 这里只注册回调函数
     */
    // IPC_Notify_registerClient() - 在系统初始化后调用

    /* 初始化数据处理链 */
    if (MSS_initDataPath() != POSE_OK) {
        gMmwMssMCB.state = MSS_STATE_ERROR;
        return POSE_ERROR_INVALID_PARAM;
    }
    
    gMmwMssMCB.state = MSS_STATE_IDLE;
    return POSE_OK;
}

PoseError_e MSS_initML(void)
{
    /* 清零特征缓冲区 */
    memset(gInputVector, 0, sizeof(gInputVector));
    memset(gInputVectorTemp, 0, sizeof(gInputVectorTemp));
    memset(gModelInput, 0, sizeof(gModelInput));
    memset(gModelOutput, 0, sizeof(gModelOutput));
    memset(gDataSet, 0, sizeof(gDataSet));
    
    /* TODO: 初始化 TVM 运行时
     * 验证模型版本和输入输出维度
     */
    
    gMmwMssMCB.mlReady = 1;
    return POSE_OK;
}

PoseError_e MSS_initDataPath(void)
{
    int32_t errCode = 0;
    
    /* 1. 初始化 Range FFT DPU (SDK 6.x API) */
    DPU_RangeProcHWA_InitParams rangeInitParams;
    memset(&rangeInitParams, 0, sizeof(rangeInitParams));
    /* HWA Handle 由 SysConfig/Drivers_open 初始化，这里临时设为 NULL */
    rangeInitParams.hwaHandle = NULL;  /* TODO: 从全局获取 gHwaHandle */
    
    gRangeProcHandle = DPU_RangeProcHWA_init(&rangeInitParams, &errCode);
    if (gRangeProcHandle == NULL) {
        DebugP_log("Range DPU init failed: %d\r\n", errCode);
        return POSE_ERROR_INVALID_PARAM;
    }
    
    DPU_RangeProcHWA_Config rangeCfg;
    memset(&rangeCfg, 0, sizeof(rangeCfg));
    /* 静态配置 */
    rangeCfg.staticCfg.numRangeBins = CFG_NUM_RANGE_BINS;
    rangeCfg.staticCfg.numChirpsPerFrame = CFG_NUM_CHIRPS_PER_FRAME;
    rangeCfg.staticCfg.numVirtualAntennas = CFG_NUM_VIRTUAL_ANTENNAS;
    rangeCfg.staticCfg.numTxAntennas = CFG_NUM_TX_ANTENNAS;
    /* 硬件资源 */
    rangeCfg.hwRes.radarCube.data = (void*)gRadarCube;
    rangeCfg.hwRes.radarCube.dataSize = sizeof(gRadarCube);
    
    errCode = DPU_RangeProcHWA_config(gRangeProcHandle, &rangeCfg);
    if (errCode != 0) {
        DebugP_log("Range DPU config failed: %d\r\n", errCode);
        return POSE_ERROR_INVALID_PARAM;
    }
    
    /* 2. 初始化 CFAR DPU (SDK 6.x API - DPU_CFARProcHWA) */
    DPU_CFARProcHWA_InitParams cfarInitParams;
    memset(&cfarInitParams, 0, sizeof(cfarInitParams));
    /* HWA Handle 由 SysConfig/Drivers_open 初始化 */
    cfarInitParams.hwaHandle = NULL;  /* TODO: 从全局获取 gHwaHandle */
    
    gCfarProcHandle = DPU_CFARProcHWA_init(&cfarInitParams, &errCode);
    if (gCfarProcHandle == NULL) {
        DebugP_log("CFAR DPU init failed: %d\r\n", errCode);
        return POSE_ERROR_INVALID_PARAM;
    }
    
    DPU_CFARProcHWA_Config cfarCfg;
    memset(&cfarCfg, 0, sizeof(cfarCfg));
    /* 静态配置 */
    cfarCfg.staticCfg.numRangeBins = CFG_NUM_RANGE_BINS;
    cfarCfg.staticCfg.numDopplerBins = CFG_NUM_DOPPLER_BINS;
    /* 动态配置 - cfarCfgRange 是指针 */
    static DPU_CFARProc_CfarCfg gCfarRangeCfg = {0};
    gCfarRangeCfg.guardLen = CFAR_GUARD_LEN;
    gCfarRangeCfg.winLen = CFAR_NOISE_LEN;
    gCfarRangeCfg.thresholdScale = (uint16_t)(powf(10.0f, CFAR_THRESHOLD_DB / 10.0f) * 256);
    cfarCfg.dynCfg.cfarCfgRange = &gCfarRangeCfg;
    /* 硬件资源 */
    cfarCfg.res.detMatrix.data = (void*)gDetectionMatrix;
    cfarCfg.res.detMatrix.dataSize = sizeof(gDetectionMatrix);
    cfarCfg.res.cfarRngDopSnrList = gCfarDetList;
    cfarCfg.res.cfarRngDopSnrListSize = MAX_NUM_DETECTED_POINTS;
    
    errCode = DPU_CFARProcHWA_config(gCfarProcHandle, &cfarCfg);
    if (errCode != 0) {
        DebugP_log("CFAR DPU config failed: %d\r\n", errCode);
        return POSE_ERROR_INVALID_PARAM;
    }
    
    /* 3. 初始化跟踪器 DPU (SDK 6.x API - 只有一个参数) */
    gTrackerHandle = DPU_TrackerProc_init(&errCode);
    if (gTrackerHandle == NULL) {
        DebugP_log("Tracker DPU init failed: %d\r\n", errCode);
        return POSE_ERROR_INVALID_PARAM;
    }
    
    DPU_TrackerProc_Config trackerCfg;
    memset(&trackerCfg, 0, sizeof(trackerCfg));
    /* SDK 6.x: TrackerProc 静态配置 */
    trackerCfg.staticCfg.trackerEnabled = 1;
    trackerCfg.staticCfg.sensorHeight = 2.0f;  /* 传感器高度 2m */
    trackerCfg.staticCfg.sensorAzimuthTilt = 0.0f;
    trackerCfg.staticCfg.sensorElevationTilt = 0.0f;
    /* GTRACK 参数 */
    trackerCfg.staticCfg.gtrackModuleConfig.maxNumPoints = MAX_NUM_DETECTED_POINTS;
    trackerCfg.staticCfg.gtrackModuleConfig.maxNumTracks = MAX_NUM_TRACKS;
    
    errCode = DPU_TrackerProc_config(gTrackerHandle, &trackerCfg);
    if (errCode != 0) {
        DebugP_log("Tracker DPU config failed: %d\r\n", errCode);
        return POSE_ERROR_INVALID_PARAM;
    }
    
    return POSE_OK;
}

/*******************************************************************************
 * 传感器控制
 ******************************************************************************/

PoseError_e MSS_sensorStart(void)
{
    int32_t retVal;
    int32_t errCode;
    MMWave_StrtCfg startCfg;  /* SDK 6.x: MMWave_StrtCfg */
    
    if (gMmwMssMCB.state == MSS_STATE_ERROR) {
        return POSE_ERROR_NOT_READY;
    }
    
    /* 配置启动参数 (SDK 6.x 结构不同) */
    memset(&startCfg, 0, sizeof(startCfg));
    startCfg.frameTrigMode = 0;  /* Software trigger */
    
    /* 启动 mmWave (SDK 6.x: 3个参数) */
    retVal = MMWave_start(gMmwaveHandle, &startCfg, &errCode);
    if (retVal != 0) {
        DebugP_log("MMWave_start failed: %d\r\n", errCode);
        return POSE_ERROR_INVALID_PARAM;
    }
    
    /* 通知 DSS 启动 (Phase 3.3) */
    PoseIpcMsg_t msg;
    msg.type = IPC_MSG_SENSOR_START;
    // Mailbox_write(gMmwMssMCB.mailboxHandle, (uint8_t*)&msg, sizeof(msg));
    
    gMmwMssMCB.sensorStarted = 1;
    gMmwMssMCB.isRunning = 1;
    gMmwMssMCB.state = MSS_STATE_RUNNING;
    gMmwMssMCB.frameCount = 0;
    
    return POSE_OK;
}

PoseError_e MSS_sensorStop(void)
{
    int32_t retVal;
    int32_t errCode;
    MMWave_StrtCfg stopCfg;  /* SDK 6.x */
    
    memset(&stopCfg, 0, sizeof(stopCfg));
    
    /* 停止 mmWave (SDK 6.x: 3个参数) */
    retVal = MMWave_stop(gMmwaveHandle, &stopCfg, &errCode);
    if (retVal != 0) {
        DebugP_log("MMWave_stop failed: %d\r\n", errCode);
        return POSE_ERROR_INVALID_PARAM;
    }
    
    gMmwMssMCB.sensorStarted = 0;
    gMmwMssMCB.isRunning = 0;
    gMmwMssMCB.state = MSS_STATE_STOPPED;
    
    return POSE_OK;
}

/*******************************************************************************
 * 帧处理
 ******************************************************************************/

PoseError_e MSS_processFrame(uint32_t frameNum)
{
    int32_t retVal = 0;
    PoseError_e poseRet = POSE_OK;
    
    if (!gMmwMssMCB.isRunning) return POSE_ERROR_NOT_READY;
    
    gMmwMssMCB.frameCount = frameNum;
    
    /* 1. Range FFT 处理 */
    DPU_RangeProcHWA_OutParams rangeOutParams;
    memset(&rangeOutParams, 0, sizeof(rangeOutParams));
    
    retVal = DPU_RangeProcHWA_process(gRangeProcHandle, &rangeOutParams);
    if (retVal != 0) return POSE_ERROR_INVALID_PARAM;
    
    /* 2. 生成检测矩阵 (对所有虚拟天线求和) */
    memset(gDetectionMatrix, 0, sizeof(gDetectionMatrix));
    for (uint32_t ant = 0; ant < CFG_NUM_VIRTUAL_ANTENNAS; ant++) {
        for (uint32_t bin = 0; bin < CFG_NUM_RANGE_BINS; bin++) {
            uint32_t sum = 0;
            for (uint32_t chirp = 0; chirp < CFG_NUM_CHIRPS_PER_FRAME; chirp++) {
                sum += gRadarCube[ant][chirp][bin];
            }
            gDetectionMatrix[ant][bin] = (uint16_t)(sum / CFG_NUM_CHIRPS_PER_FRAME);
        }
    }
    
    /* 3. CFAR 检测 (SDK 6.x API - DPU_CFARProcHWA) */
    DPU_CFARProcHWA_OutParams cfarOutParams;
    memset(&cfarOutParams, 0, sizeof(cfarOutParams));
    
    retVal = DPU_CFARProcHWA_process(gCfarProcHandle, &cfarOutParams);
    if (retVal != 0) return POSE_ERROR_INVALID_PARAM;
    
    uint32_t numDetected = cfarOutParams.numCfarDetectedPoints;
    
    /* 转换 CFAR 输出为点云格式 */
    /* SDK 6.x: CFAR 输出在配置时指定的 cfarRngDopSnrList 缓冲区 (gCfarDetList) */
    for (uint32_t i = 0; i < numDetected && i < MAX_NUM_DETECTED_POINTS; i++) {
        DPIF_CFARDetList* cfarObj = &gCfarDetList[i];
        float range = (float)cfarObj->rangeIdx * CFG_RANGE_RESOLUTION_M;
        float doppler = (float)cfarObj->dopplerIdx * CFG_VELOCITY_RESOLUTION_MPS;
        
        gPointCloudBuf[i].x = range; /* 简化: 假设正前方 */
        gPointCloudBuf[i].y = 0.0f;
        gPointCloudBuf[i].z = 0.0f;
        gPointCloudBuf[i].velocity = doppler;
        gPointCloudBuf[i].snr = (float)cfarObj->snr;
    }
    gMmwMssMCB.pointCloud = gPointCloudBuf;
    gMmwMssMCB.numDetectedPoints = numDetected;
    
    /* 4. 目标跟踪 (SDK 6.x API) */
    if (numDetected >= ML_TYPE3_FEATURE_MIN_COUNT) {
        DPU_TrackerProc_OutParams trackerOutParams;
        memset(&trackerOutParams, 0, sizeof(trackerOutParams));
        
        /* 转换点云为扩展格式 */
        for (uint32_t i = 0; i < numDetected && i < MAX_NUM_DETECTED_POINTS; i++) {
            gPointCloudExt[i].x = gPointCloudBuf[i].x;
            gPointCloudExt[i].y = gPointCloudBuf[i].y;
            gPointCloudExt[i].z = gPointCloudBuf[i].z;
            gPointCloudExt[i].velocity = gPointCloudBuf[i].velocity;
            gPointCloudExt[i].snr = gPointCloudBuf[i].snr;
        }
        
        /* SDK 6.x: DPU_TrackerProc_process(handle, numObjsIn, pointCloud, outParams) */
        retVal = DPU_TrackerProc_process(gTrackerHandle, numDetected, gPointCloudExt, &trackerOutParams);
        if (retVal == 0) {
            gMmwMssMCB.trackerTargets = gTrackerTargetsBuf;
            gMmwMssMCB.numTrackedTargets = trackerOutParams.numTargets;
        }
    }
    
    /* 5. 特征提取 & 6. ML 推理 */
    if (gMmwMssMCB.numTrackedTargets > 0 && 
        gMmwMssMCB.numDetectedPoints >= ML_TYPE3_FEATURE_MIN_COUNT) {
        
        float features[ML_TYPE3_FEATURE_COUNT];
        poseRet = MSS_extractFeatures(
            gMmwMssMCB.pointCloud, gMmwMssMCB.numDetectedPoints,
            &gMmwMssMCB.trackerTargets[0], features
        );
        
        if (poseRet == POSE_OK) {
            MSS_updateFeatureBuffer(features);
        }
    }
    
    if (MSS_canRunInference()) {
        PoseClassResult_t result;
        poseRet = MSS_runInference(&result);
        
        if (poseRet == POSE_OK && result.isValid) {
            memcpy(&gMmwMssMCB.classResult, &result, sizeof(PoseClassResult_t));
            gMmwMssMCB.mlInferenceCount++;
            MSS_sendMLResult(&result);
#if ML_DEBUG_PRINT_ENABLED
            MSS_debugPrintResult(&result);
#endif
        } else {
            gMmwMssMCB.mlErrorCount++;
        }
    }
    
    /* 7. 发送其他数据到 UART */
    MSS_sendFrameHeader(frameNum, gMmwMssMCB.numDetectedPoints);
    // MSS_sendPointCloud(gMmwMssMCB.pointCloud, gMmwMssMCB.numDetectedPoints);
    // MSS_sendTrackerTargets(gMmwMssMCB.trackerTargets, gMmwMssMCB.numTrackedTargets);
    
    return POSE_OK;
}

/*******************************************************************************
 * 特征提取 (移植自 IWRL6432 pose.c)
 ******************************************************************************/

PoseError_e MSS_extractFeatures(
    const PointCloud_t* points,
    uint32_t numPoints,
    const TrackerTarget_t* target,
    float* features)
{
    uint32_t i;
    PointCloud_t maxHeightPoints[ML_MAX_HEIGHT_POINTS];
    uint32_t numSelected;
    
    if (points == NULL || features == NULL || numPoints < ML_TYPE3_FEATURE_MIN_COUNT) {
        return POSE_ERROR_INSUFFICIENT_POINTS;
    }
    
    /* 清零特征数组 */
    memset(features, 0, ML_TYPE3_FEATURE_COUNT * sizeof(float));
    
    /* 1. 提取运动特征 (7个) */
    if (target != NULL) {
        features[0] = target->posZ;     /* posz */
        features[1] = target->velX;     /* velx */
        features[2] = target->velY;     /* vely */
        features[3] = target->velZ;     /* velz */
        features[4] = target->accX;     /* accx */
        features[5] = target->accY;     /* accy */
        features[6] = target->accZ;     /* accz */
    }
    
    /* 2. 选择最高的5个点 */
    numSelected = MSS_selectMaxHeightPoints(
        points, numPoints, 
        maxHeightPoints, ML_MAX_HEIGHT_POINTS
    );
    
    /* 3. 提取点云特征 (15个 = 5点 × 3属性) */
    for (i = 0; i < numSelected && i < ML_MAX_HEIGHT_POINTS; i++) {
        uint32_t baseIdx = 7 + i * 3;   /* 从索引7开始 */
        features[baseIdx + 0] = maxHeightPoints[i].y;   /* y */
        features[baseIdx + 1] = maxHeightPoints[i].z;   /* z */
        features[baseIdx + 2] = maxHeightPoints[i].snr; /* snr */
    }
    
    return POSE_OK;
}

uint32_t MSS_selectMaxHeightPoints(
    const PointCloud_t* points,
    uint32_t numPoints,
    PointCloud_t* selectedPoints,
    uint32_t maxSelect)
{
    uint32_t i, j;
    float maxZ[ML_MAX_HEIGHT_POINTS];
    int32_t maxIdx[ML_MAX_HEIGHT_POINTS];
    uint32_t numSelected = 0;
    
    /* 初始化 */
    for (i = 0; i < maxSelect; i++) {
        maxZ[i] = -1e10f;
        maxIdx[i] = -1;
    }
    
    /* 找出最高的 maxSelect 个点 */
    for (i = 0; i < numPoints; i++) {
        float z = points[i].z;
        
        /* 插入排序 */
        for (j = 0; j < maxSelect; j++) {
            if (z > maxZ[j]) {
                /* 移动后面的元素 */
                for (uint32_t k = maxSelect - 1; k > j; k--) {
                    maxZ[k] = maxZ[k-1];
                    maxIdx[k] = maxIdx[k-1];
                }
                maxZ[j] = z;
                maxIdx[j] = (int32_t)i;
                break;
            }
        }
    }
    
    /* 复制选中的点 */
    for (i = 0; i < maxSelect; i++) {
        if (maxIdx[i] >= 0) {
            memcpy(&selectedPoints[numSelected], &points[maxIdx[i]], sizeof(PointCloud_t));
            numSelected++;
        }
    }
    
    return numSelected;
}

PoseError_e MSS_updateFeatureBuffer(const float* features)
{
    uint32_t i;
    
    if (features == NULL) {
        return POSE_ERROR_INVALID_PARAM;
    }
    
    /* 
     * 循环缓冲区操作 (来自 IWRL6432 pose.c CreateFeatureVector)
     * 移动旧帧数据，为新帧腾出空间
     */
    for (i = 0; i < NUM_FRAMES_OF_DATA * (ML_TYPE3_FEATURE_COUNT - 1); i++) {
        gInputVectorTemp[i] = gInputVectorTemp[i + ML_TYPE3_FEATURE_COUNT];
    }
    
    /* 添加最新帧数据 */
    for (i = 0; i < ML_TYPE3_FEATURE_COUNT; i++) {
        gInputVectorTemp[NUM_FRAMES_OF_DATA * ML_TYPE3_FEATURE_COUNT - ML_TYPE3_FEATURE_COUNT + i] = 
            (int32_t)(features[i] * 1000.0f);  /* 缩放为整数 */
    }
    
    /* 更新帧计数 */
    if (gMmwMssMCB.featureVector.frameCount < NUM_FRAMES_OF_DATA) {
        gMmwMssMCB.featureVector.frameCount++;
    }
    
    /* 检查是否有足够的帧 */
    if (gMmwMssMCB.featureVector.frameCount >= NUM_FRAMES_OF_DATA) {
        gMmwMssMCB.featureVector.isValid = 1;
    }
    
    return POSE_OK;
}

/*******************************************************************************
 * 特征向量构建 (移植自 IWRL6432 pose.c)
 ******************************************************************************/

/**
 * @brief 构建用于模型推理的特征向量
 * 
 * 将8帧 × 22特征的数据交织重排为176维输入
 */
static void CreateFeatureVector(void)
{
    uint32_t i, j, k;
    
    /* 交织帧数据 */
    j = 0;
    k = 0;
    for (i = 0; i < NUM_FRAMES_OF_DATA * ML_TYPE3_FEATURE_COUNT; i++) {
        gInputVector[i] = gInputVectorTemp[k * ML_TYPE3_FEATURE_COUNT + j];
        if (k == NUM_FRAMES_OF_DATA - 1) {
            k = 0;
            j++;
        } else {
            k++;
        }
    }
    
    /* 转换为 float 并复制到模型输入 */
    for (i = 0; i < NUM_FRAMES_OF_DATA * ML_TYPE3_FEATURE_COUNT; i++) {
        gModelInput[i] = (float)gInputVector[i] / 1000.0f;
    }
    
    /* 更新特征向量结构 */
    memcpy(gMmwMssMCB.featureVector.data, gModelInput, sizeof(gModelInput));
}

/*******************************************************************************
 * ML 推理 (移植自 IWRL6432 pose.c)
 ******************************************************************************/

uint8_t MSS_canRunInference(void)
{
    return (gMmwMssMCB.mlReady && 
            gMmwMssMCB.featureVector.isValid &&
            gMmwMssMCB.featureVector.frameCount >= NUM_FRAMES_OF_DATA);
}

PoseError_e MSS_runInference(PoseClassResult_t* result)
{
    int32_t retVal;
    uint32_t i;
    float maxProb = -1.0f;
    uint32_t maxIdx = 0;
    
    if (result == NULL) {
        return POSE_ERROR_INVALID_PARAM;
    }
    
    if (!MSS_canRunInference()) {
        return POSE_ERROR_NOT_READY;
    }
    
    /* 1. 构建特征向量 */
    CreateFeatureVector();
    
    /* 2. 执行模型推理 */
    retVal = RunTVMModel(gModelOutput);
    
    if (retVal != 0) {
        result->isValid = 0;
        return POSE_ERROR_MODEL_FAILED;
    }
    
    /* 3. 解析输出结果 */
    for (i = 0; i < ML_TYPE_HUMANPOSE_CLASSES; i++) {
        result->allProbs[i] = gModelOutput[i];
        if (gModelOutput[i] > maxProb) {
            maxProb = gModelOutput[i];
            maxIdx = i;
        }
    }
    
    result->classIndex = (PoseClassIndex_e)maxIdx;
    result->confidence = maxProb;
    result->isValid = (maxProb >= CLASS_THRESHOLD) ? 1 : 0;
    
    return POSE_OK;
}

/**
 * @brief 执行 TVM 模型推理
 * @param[out] outputBuf 输出缓冲区
 * @return 0 成功, -1 失败
 */
static int32_t RunTVMModel(float* outputBuf)
{
#ifdef USE_TVM_MODEL
    /*
     * 使用 TVM 编译的模型 (pose_model_r4f.a)
     * 
     * 需要在 CCS 项目中:
     * 1. 链接 pose_model_r4f.a
     * 2. 定义 USE_TVM_MODEL 宏
     */
    #include "../model/pose_model_wrapper.h"
    
    return PoseModel_run(gModelInput, outputBuf);
    
#else
    /*
     * 模拟输出 (用于测试框架，无需实际模型)
     * 
     * 当 USE_TVM_MODEL 未定义时，返回模拟的分类结果
     * 这允许在没有编译好的模型库时测试整个数据流
     */
    
    /* 模拟: 根据输入特征简单判断 */
    float sumZ = 0.0f;
    float sumVel = 0.0f;
    uint32_t i;
    
    /* 计算 Z 轴位置和速度的平均值 */
    for (i = 0; i < NUM_FRAMES_OF_DATA; i++) {
        uint32_t baseIdx = i * ML_TYPE3_FEATURE_COUNT;
        sumZ += gModelInput[baseIdx + 0];   /* posz */
        sumVel += fabsf(gModelInput[baseIdx + 1]) + 
                  fabsf(gModelInput[baseIdx + 2]) + 
                  fabsf(gModelInput[baseIdx + 3]); /* vel */
    }
    sumZ /= NUM_FRAMES_OF_DATA;
    sumVel /= NUM_FRAMES_OF_DATA;
    
    /* 简单规则分类 (仅用于测试) */
    if (sumZ > 1.5f) {
        /* 高位置 -> 站立 */
        outputBuf[0] = 0.8f;   /* Standing */
        outputBuf[1] = 0.1f;   /* Sitting */
        outputBuf[2] = 0.05f;  /* Lying */
        outputBuf[3] = 0.03f;  /* Falling */
        outputBuf[4] = 0.02f;  /* Walking */
    } else if (sumZ > 0.8f) {
        /* 中等位置 -> 坐下 */
        outputBuf[0] = 0.1f;
        outputBuf[1] = 0.75f;
        outputBuf[2] = 0.1f;
        outputBuf[3] = 0.03f;
        outputBuf[4] = 0.02f;
    } else if (sumVel > 0.5f) {
        /* 低位置 + 高速度 -> 跌倒 */
        outputBuf[0] = 0.05f;
        outputBuf[1] = 0.05f;
        outputBuf[2] = 0.1f;
        outputBuf[3] = 0.75f;
        outputBuf[4] = 0.05f;
    } else {
        /* 低位置 + 低速度 -> 躺下 */
        outputBuf[0] = 0.05f;
        outputBuf[1] = 0.1f;
        outputBuf[2] = 0.8f;
        outputBuf[3] = 0.03f;
        outputBuf[4] = 0.02f;
    }
    
    return 0;
#endif
}

/*******************************************************************************
 * UART 输出
 ******************************************************************************/

PoseError_e MSS_sendMLResult(const PoseClassResult_t* result)
{
    if (result == NULL || !result->isValid) {
        return POSE_ERROR_INVALID_PARAM;
    }
    
    /* 填充 TLV 结构 */
    gClassificationResult.mlClassType = ML_TYPE_HUMANPOSE;
    gClassificationResult.mlClassCount = ML_TYPE_HUMANPOSE_CLASSES;
    memcpy(gClassificationResult.mlResult, result->allProbs, 
           sizeof(float) * ML_TYPE_HUMANPOSE_CLASSES);
    
    /* 
     * TODO: 发送 TLV 到 UART
     * 
     * TLV 格式:
     * - Type: MMWDEMO_OUTPUT_MSG_ML_CLASSIFICATION_PROBABILITY (1031)
     * - Length: sizeof(MmwDemo_output_message_ml)
     * - Value: gClassificationResult
     */
    
    return POSE_OK;
}

PoseError_e MSS_sendFrameHeader(uint32_t frameNum, uint32_t numPoints)
{
    /* 
     * TODO: 发送帧头 TLV
     */
    (void)frameNum;
    (void)numPoints;
    return POSE_OK;
}

/*******************************************************************************
 * 调试函数
 ******************************************************************************/

#if ML_DEBUG_PRINT_ENABLED
void MSS_debugPrintFeatures(const float* features, uint32_t numFeatures)
{
    uint32_t i;
    printf("Features [%u]: ", numFeatures);
    for (i = 0; i < numFeatures && i < 10; i++) {
        printf("%.3f ", features[i]);
    }
    if (numFeatures > 10) {
        printf("...");
    }
    printf("\n");
}

void MSS_debugPrintResult(const PoseClassResult_t* result)
{
    printf("Classification: %s (%.2f%%)\n",
           POSE_CLASS_NAMES[result->classIndex],
           result->confidence * 100.0f);
    printf("  All probs: [");
    for (uint32_t i = 0; i < POSE_CLASS_COUNT; i++) {
        printf("%.2f", result->allProbs[i]);
        if (i < POSE_CLASS_COUNT - 1) printf(", ");
    }
    printf("]\n");
}
#endif
