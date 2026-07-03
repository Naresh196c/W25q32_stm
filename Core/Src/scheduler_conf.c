#include "scheduler.h"

#include "battery_broadcast.h"
#include "battery_can_rx.h"
#include "nvm_config.h"

#define TOTAL_TASK       3U
#define TOTAL_INIT_TASK  1U

const uint8_t Total_task_u8 = TOTAL_TASK;
const uint8_t Total_Init_task_u8 = TOTAL_INIT_TASK;

static bool Scheduler_ModuleInit(void)
{
    BatteryCAN_Init();
    BatteryBroadcast_Init();
    NVM_BatteryCfg_Init();
    return true;
}

TaskConf_St_t TaskConf_St[TOTAL_TASK] =
{
    { BatteryCAN_Task,       TS_PERIODICTY(100U), INIT_VALUE },
    { BatteryBroadcast_Task, TS_PERIODICTY(100U), INIT_VALUE },
    { NVM_BatteryCfg_Task,   TS_PERIODICTY(100U), INIT_VALUE },
};

InitTaskConf_St_t InitTaskConf_St[TOTAL_INIT_TASK] =
{
    { Scheduler_ModuleInit, 0U },
};
