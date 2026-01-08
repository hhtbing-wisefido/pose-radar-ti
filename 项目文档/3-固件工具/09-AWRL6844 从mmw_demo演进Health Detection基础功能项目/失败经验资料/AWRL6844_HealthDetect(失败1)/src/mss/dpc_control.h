/**
 * @file dpc_control.h
 * @brief DPC Control Module Header
 */

#ifndef DPC_CONTROL_H
#define DPC_CONTROL_H

#include <stdint.h>

/* DPC Control API */
int32_t DPC_Control_init(void);
int32_t DPC_Control_start(uint32_t frameNum);
int32_t DPC_Control_waitCompletion(uint32_t timeoutMs);
int32_t DPC_Control_stop(void);

/* IPC Message Types (MSS ↔ DSS) */
typedef enum {
    DPC_MSG_START = 1,
    DPC_MSG_STOP = 2,
    DPC_MSG_COMPLETE = 3,
    DPC_MSG_ERROR = 4,
} DPC_MessageType_e;

#endif /* DPC_CONTROL_H */
