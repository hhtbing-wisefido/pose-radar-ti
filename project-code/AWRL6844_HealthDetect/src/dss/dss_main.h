/**
 * @file dss_main.h
 * @brief DSS侧主程序头文件（C66x DSP）
 * 
 * 参考：第3章 3.3节 DSS算法扩展架构设计
 * 
 * DSS的职责：
 * 1. 接收MSS的DPC配置（从共享RAM）
 * 2. 执行标准DPC流程（Range/Doppler/CFAR/AOA）
 * 3. 🔥调用特征提取（第3章核心创新）
 * 4. 将特征写入共享RAM
 * 5. 通知MSS完成
 */

#ifndef DSS_MAIN_H
#define DSS_MAIN_H

#include <stdint.h>
#include "common/health_detect_types.h"

/**************************************************************************
 * DSS主函数
 **************************************************************************/

/**
 * @brief DSS侧初始化
 */
void DSS_HealthDetect_init(void);

/**
 * @brief DSS侧DPC执行
 * 
 * 扩展标准DPC流程：
 * 1. 标准DPC: Range FFT → Doppler FFT → CFAR → AOA (得到点云)
 * 2. 🔥扩展: 特征提取 (计算质心/分布/速度)
 * 3. 写入共享RAM
 * 4. 通知MSS
 */
void DSS_HealthDetect_execute(void);

#endif /* DSS_MAIN_H */
