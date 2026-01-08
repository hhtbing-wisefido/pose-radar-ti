/**
 * @file health_detect_types.h
 * @brief 健康检测专用数据类型（MSS和DSS共享）
 * 
 * 参考：第3章 3.4.2节 common/目录说明
 */

#ifndef HEALTH_DETECT_TYPES_H
#define HEALTH_DETECT_TYPES_H

#include <stdint.h>

/**************************************************************************
 * 点云特征结构 - 第3章 3.3.3节设计
 * DSS计算并写入共享RAM，MSS读取用于决策
 **************************************************************************/
typedef struct HealthDetect_PointCloudFeatures
{
    /* 质心坐标（米） */
    float centerX;
    float centerY;
    float centerZ;
    
    /* 分布范围（米） */
    float spreadXY;       /* XY平面分布 */
    float spreadZ;        /* Z轴分布 */
    
    /* 速度特征（m/s） */
    float avgVelocity;    /* 平均速度 */
    float maxVelocity;    /* 最大速度 */
    float velocityX;      /* X轴速度分量 */
    float velocityY;      /* Y轴速度分量 */
    float velocityZ;      /* Z轴速度分量 */
    
    /* 点云统计 */
    uint32_t numPoints;   /* 检测到的点数 */
    
    /* 帧信息 */
    uint32_t frameNumber;
    
} HealthDetect_PointCloudFeatures_t;

/**************************************************************************
 * 人存状态枚举 - 第4章使用
 **************************************************************************/
typedef enum {
    PRESENCE_ABSENT = 0,    /* 无人 */
    PRESENCE_PRESENT = 1,   /* 有人 */
    PRESENCE_MOTION = 2     /* 运动中 */
} PresenceState_e;

/**************************************************************************
 * 姿态状态枚举 - 第5章使用
 **************************************************************************/
typedef enum {
    POSE_UNKNOWN = 0,       /* 未知 */
    POSE_STANDING = 1,      /* 站立 */
    POSE_SITTING = 2,       /* 坐 */
    POSE_LYING = 3,         /* 躺 */
    POSE_WALKING = 4        /* 走 */
} PoseState_e;

/**************************************************************************
 * 跌倒状态枚举 - 第6章使用
 **************************************************************************/
typedef enum {
    FALL_NONE = 0,          /* 正常 */
    FALL_SUSPECTED = 1,     /* 疑似跌倒 */
    FALL_CONFIRMED = 2      /* 确认跌倒 */
} FallState_e;

#endif /* HEALTH_DETECT_TYPES_H */
