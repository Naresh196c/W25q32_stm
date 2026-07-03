#include "scheduler.h"

#include "stm32f1xx_hal.h"

static uint32_t g_lastSchedulerTick;

void Scheduler_Init(void)
{
    g_lastSchedulerTick = HAL_GetTick();
}

void Scheduler_Tick(void)
{
    for (uint8_t i = 0U; i < Total_task_u8; i++)
    {
        TaskConf_St[i].Counter++;
    }
}

void Scheduler_PollTick(void)
{
    uint32_t now = HAL_GetTick();

    while ((now - g_lastSchedulerTick) >= TS_INTERVAL_MS)
    {
        Scheduler_Tick();
        g_lastSchedulerTick += TS_INTERVAL_MS;
    }
}

void Scheduler_Run(void)
{
    for (uint8_t i = 0U; i < Total_task_u8; i++)
    {
        if (TaskConf_St[i].Counter >= TaskConf_St[i].Periodicity)
        {
            TaskConf_St[i].Counter = 0U;
            TaskConf_St[i].Task();
        }
    }
}

bool Scheduler_RunInitTasks(void)
{
    for (uint8_t i = 0U; i < Total_Init_task_u8; i++)
    {
        if (InitTaskConf_St[i].Executed == 0U)
        {
            if (InitTaskConf_St[i].Task() == false)
            {
                return false;
            }

            InitTaskConf_St[i].Executed = 1U;
        }
    }

    return true;
}
