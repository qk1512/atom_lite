#ifndef TASK_SCHEDULE_H
#define TASK_SCHEDULE_H

#include <Arduino.h>
#include "./tasks/taskButtonControlRGB.h"
#include "stdint.h"
#include <stdlib.h> // For malloc and free

#define SCH_MAX_TASKS 10
#define NO_TASK_ID 0

extern TaskHandle_t TaskScheduler;

#ifdef __cplusplus
extern "C"
{
#endif

    void SCH_Init(void);
    void SCH_Update(void);
    uint32_t SCH_Add_Task(void (*p_function)(), uint32_t DELAY, uint32_t PERIOD);
    void SCH_Dispatch_Tasks(void);
    uint8_t SCH_Delete_Task(uint32_t TASK_ID);

#ifdef __cplusplus
}
#endif

void initSetUpScheduler();

#endif
