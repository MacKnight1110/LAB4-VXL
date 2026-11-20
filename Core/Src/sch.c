#include "sch.h"

#include <stddef.h>
#include <string.h>

typedef struct
{
    SCH_TaskFunction function;
    uint32_t id;
    uint32_t deadline;
    uint32_t period;
} SCH_Task;

static SCH_Task task_queue[SCH_MAX_TASKS];
static volatile uint32_t tick_count;
static uint32_t next_task_id;
static uint32_t task_count;

static uint8_t time_reached(uint32_t now, uint32_t deadline)
{
    return ((int32_t)(now - deadline) >= 0) ? 1U : 0U;
}

static uint8_t task_before(const SCH_Task *left, const SCH_Task *right)
{
    int32_t difference = (int32_t)(left->deadline - right->deadline);

    if (difference < 0)
    {
        return 1U;
    }

    if (difference > 0)
    {
        return 0U;
    }

    /* Keep equal-deadline callbacks in registration order. */
    return (left->id < right->id) ? 1U : 0U;
}

static void insert_task(const SCH_Task *task)
{
    uint32_t index = task_count;

    while ((index > 0U) && task_before(task, &task_queue[index - 1U]))
    {
        task_queue[index] = task_queue[index - 1U];
        index--;
    }

    task_queue[index] = *task;
    task_count++;
}

static SCH_Task remove_task_at(uint32_t index)
{
    SCH_Task removed = task_queue[index];
    uint32_t move_index;

    for (move_index = index; move_index + 1U < task_count; move_index++)
    {
        task_queue[move_index] = task_queue[move_index + 1U];
    }

    task_count--;
    return removed;
}

void SCH_Init(void)
{
    memset(task_queue, 0, sizeof(task_queue));
    tick_count = 0U;
    next_task_id = 1U;
    task_count = 0U;
}

uint32_t get_time(void)
{
    return tick_count * SCH_TICK_MS;
}

uint32_t SCH_Add_Task(SCH_TaskFunction function,
                      uint32_t delay_ticks,
                      uint32_t period_ticks)
{
    SCH_Task task;

    if ((function == NULL) || (task_count >= SCH_MAX_TASKS))
    {
        return SCH_INVALID_TASK_ID;
    }

    task.function = function;
    task.id = next_task_id++;
    if (next_task_id == SCH_INVALID_TASK_ID)
    {
        next_task_id = 1U;
    }

    task.deadline = tick_count + delay_ticks;
    task.period = period_ticks;

    insert_task(&task);
    return task.id;
}

uint8_t SCH_Delete_Task(uint32_t task_id)
{
    uint32_t index;

    if (task_id == SCH_INVALID_TASK_ID)
    {
        return 1U;
    }

    for (index = 0U; index < task_count; index++)
    {
        if (task_queue[index].id == task_id)
        {
            (void)remove_task_at(index);
            return 0U;
        }
    }

    return 1U;
}

void SCH_Update(void)
{
    /* O(1): the ISR only advances the scheduler clock. */
    tick_count++;
}

void SCH_Dispatch_Tasks(void)
{
    while (task_count > 0U)
    {
        uint32_t now = tick_count;
        SCH_Task task;

        if (time_reached(now, task_queue[0].deadline) == 0U)
        {
            break;
        }

        task = remove_task_at(0U);
        task.function();

        if (task.period > 0U)
        {
            /* Keep the task's original phase and preserve deadline ordering. */
            task.deadline += task.period;
            insert_task(&task);
        }
    }
}