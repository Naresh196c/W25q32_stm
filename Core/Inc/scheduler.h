#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdbool.h>
#include <stdint.h>

#define TS_INTERVAL_MS      5U
#define TS_PERIODICTY(ms)   ((uint16_t)((ms) / TS_INTERVAL_MS))
#define INIT_VALUE          0U

typedef void (*SchedulerTaskFn_t)(void);
typedef bool (*SchedulerInitTaskFn_t)(void);

typedef struct
{
    SchedulerTaskFn_t Task;
    uint16_t Periodicity;
    uint16_t Counter;
} TaskConf_St_t;

typedef struct
{
    SchedulerInitTaskFn_t Task;
    uint8_t Executed;
} InitTaskConf_St_t;

extern const uint8_t Total_task_u8;
extern const uint8_t Total_Init_task_u8;
extern TaskConf_St_t TaskConf_St[];
extern InitTaskConf_St_t InitTaskConf_St[];

void Scheduler_Init(void);
void Scheduler_Tick(void);
void Scheduler_PollTick(void);
void Scheduler_Run(void);
bool Scheduler_RunInitTasks(void);

#endif
