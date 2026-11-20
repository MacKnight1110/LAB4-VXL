#ifndef SCH_H
#define SCH_H

#include <stdint.h>

#define SCH_MAX_TASKS       16U
#define SCH_INVALID_TASK_ID 0U
#define SCH_TICK_MS         10U

typedef void (*SCH_TaskFunction)(void);

/* Returns SCH_INVALID_TASK_ID if the callback is null or the queue is full. */
uint32_t SCH_Add_Task(SCH_TaskFunction function,
                      uint32_t delay_ticks,
                      uint32_t period_ticks);

/* Returns 0 when deleted, 1 when the ID was not found. */
uint8_t SCH_Delete_Task(uint32_t task_id);

/* Call only from the 10 ms timer interrupt. */
void SCH_Update(void);

/* Call repeatedly from the main loop. */
void SCH_Dispatch_Tasks(void);

/* Milliseconds since SCH_Init(), with 10 ms resolution. */
uint32_t get_time(void);

void SCH_Init(void);

#endif