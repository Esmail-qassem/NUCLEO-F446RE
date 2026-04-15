#ifndef SWM_CFG_H_
#define SWM_CFG_H_

#include "STD_TYPES.h"

/*==================================================================
 *  SwM_Cfg.h
 *  Scheduler task table and software-module tunables.
 *================================================================*/

/*------------------------------------------------------------------
 *  Task identifiers (= scheduler priority, 0 highest)
 *------------------------------------------------------------------*/
typedef enum
{
    TASK_ID_5MS    = 0U,
    TASK_ID_10MS   = 1U,
    TASK_ID_20MS   = 2U,
    TASK_ID_50MS   = 3U,
    TASK_ID_100MS  = 4U,
    TASK_ID_1000MS = 5U,
    TASK_ID_COUNT  = 6U
} SwM_TaskId_t;

/*------------------------------------------------------------------
 *  Task periods (ms)
 *------------------------------------------------------------------*/
#define TASK_PERIOD_5MS         (5U)
#define TASK_PERIOD_10MS        (10U)
#define TASK_PERIOD_20MS        (20U)
#define TASK_PERIOD_50MS        (50U)
#define TASK_PERIOD_100MS       (100U)
#define TASK_PERIOD_1000MS      (1000U)

/*------------------------------------------------------------------
 *  Telemetry sub-rates inside the 1000 ms task (seconds)
 *------------------------------------------------------------------*/
#define TLM_TEMP_PERIOD_S       (5U)
#define TLM_RUNTIME_PERIOD_S    (10U)
#define TLM_STACK_PERIOD_S      (30U)
#define TLM_TICK_WRAP_S         (60U)   /* LCM of the periods above */

/*------------------------------------------------------------------
 *  Accelerometer → OLED pixel mapping
 *------------------------------------------------------------------*/
#define OLED_CENTER_X           (64)    /* OLED_WIDTH  / 2 */
#define OLED_CENTER_Y           (32)    /* OLED_HEIGHT / 2 */
#define ACCEL_FULL_SCALE_CG     (100)   /* ±1.00 g expressed in centi-g */

/*------------------------------------------------------------------
 *  Task configuration entry
 *------------------------------------------------------------------*/
typedef struct
{
    uint8   priority;
    uint16  period_ms;
    void  (*handler)(void);
} SwM_TaskCfg_t;

extern const SwM_TaskCfg_t SwM_TaskTable[TASK_ID_COUNT];

#endif /* SWM_CFG_H_ */
