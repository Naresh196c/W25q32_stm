#ifndef NVM_CONFIG_H
#define NVM_CONFIG_H

#include <stdint.h>

#define MAX_BATTERIES                    8U

#define NVM_BATTERY_BLOCK_ID             0U
#define NVM_BATTERY_BLOCK_SIZE_BYTES     64U
#define NVM_BATTERY_MAGIC                0xA55AU
#define NVM_BATTERY_VERSION              1U

#define NVM_BATTERY_CAN_STATUS_ID        0x520U
#define NVM_BATTERY_CAN_DATA1_ID         0x521U
#define NVM_BATTERY_CAN_DATA2_ID         0x522U
#define NVM_BATTERY_CAN_DATA3_ID         0x523U
#define NVM_BATTERY_CAN_DATA4_ID         0x524U
#define NVM_BATTERY_CAN_ACK_ID           0x532U
#define NVM_BATTERY_CAN_RX_SET_ID        0x530U
#define NVM_BATTERY_CAN_RX_SAVE_ID       0x531U

#define NVM_PARAM_S1_BTRY1_ID            1U
#define NVM_PARAM_S1_BTRY2_ID            2U
#define NVM_PARAM_S2_BTRY1_ID            3U
#define NVM_PARAM_S2_BTRY2_ID            4U
#define NVM_PARAM_S3_BTRY1_ID            5U
#define NVM_PARAM_S3_BTRY2_ID            6U
#define NVM_PARAM_S4_BTRY1_ID            7U
#define NVM_PARAM_S4_BTRY2_ID            8U

#pragma pack(push, 1)
typedef struct
{
    uint16_t magic;
    uint8_t version;
    uint8_t block_id;
    uint16_t save_count;
    uint16_t checksum;

    uint32_t s1_btry1_id;
    uint32_t s1_btry2_id;
    uint32_t s2_btry1_id;
    uint32_t s2_btry2_id;
    uint32_t s3_btry1_id;
    uint32_t s3_btry2_id;
    uint32_t s4_btry1_id;
    uint32_t s4_btry2_id;

    uint8_t reserved[24];
} NVM_BatteryCfg_t;
#pragma pack(pop)

void NVM_BatteryCfg_Init(void);
void NVM_BatteryCfg_Task(void);
void NVM_BatteryCfg_LoadDefaults(void);
uint8_t NVM_BatteryCfg_Load(void);
uint8_t NVM_BatteryCfg_Save(void);
uint8_t NVM_BatteryCfg_IsValid(void);
uint8_t NVM_BatteryCfg_SetParam(uint8_t param_id, uint32_t value);
void NVM_BatteryCfg_SaveCommand(void);
void NVM_BatteryCfg_CanRx(uint32_t can_id, const uint8_t *data, uint8_t dlc);
const NVM_BatteryCfg_t *NVM_BatteryCfg_Get(void);
uint32_t NVM_BatteryCfg_GetSlotId(uint8_t slot);

#endif
