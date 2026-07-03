#include "nvm_config.h"

#include "mcp2515.h"
#include "w25q32.h"

#include <stdio.h>
#include <string.h>

#define NVM_W25Q_BASE_ADDRESS   0x000000UL
#define W25Q32_JEDEC_ID         0xEF4016UL

typedef enum
{
    NVM_BATTERY_STATE_IDLE = 0,
    NVM_BATTERY_STATE_LOAD,
    NVM_BATTERY_STATE_VALIDATE,
    NVM_BATTERY_STATE_DEFAULT_SAVE,
    NVM_BATTERY_STATE_TX_STATUS,
    NVM_BATTERY_STATE_TX_DATA,
    NVM_BATTERY_STATE_SAVE_CMD,
    NVM_BATTERY_STATE_DONE
} NVM_BatteryState_t;

static NVM_BatteryCfg_t g_batteryCfg;
static uint8_t g_nvmReadStatus;
static uint8_t g_nvmWriteStatus;
static uint8_t g_nvmValid;
static uint8_t g_txFrameIndex;
static uint32_t g_w25JedecId;
static uint8_t g_w25Ready;
static NVM_BatteryState_t g_batteryState = NVM_BATTERY_STATE_IDLE;

static uint16_t NVM_BatteryCfg_CalcChecksum(const NVM_BatteryCfg_t *cfg)
{
    const uint8_t *raw = (const uint8_t *)cfg;
    uint16_t sum = 0U;

    for (uint8_t i = 0U; i < NVM_BATTERY_BLOCK_SIZE_BYTES; i++)
    {
        if ((i != 6U) && (i != 7U))
        {
            sum = (uint16_t)(sum + raw[i]);
        }
    }

    return sum;
}

static void U32_ToLe(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)(value & 0xFFU);
    data[1] = (uint8_t)((value >> 8U) & 0xFFU);
    data[2] = (uint8_t)((value >> 16U) & 0xFFU);
    data[3] = (uint8_t)((value >> 24U) & 0xFFU);
}

static uint32_t Le_ToU32(const uint8_t *data)
{
    return ((uint32_t)data[0]) |
           ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static HAL_StatusTypeDef NVM_BatteryCfg_SendFrame(uint16_t can_id,
                                                  const uint8_t *data)
{
    MCP2515_Frame_t frame = {0};

    frame.id = can_id;
    frame.is_extended = 0U;
    frame.dlc = 8U;
    memcpy(frame.data, data, 8U);
    return MCP2515_Send(&frame);
}

static void NVM_BatteryCfg_TxStatus(void)
{
    uint8_t data[8] = {0U};

    data[0] = (uint8_t)g_batteryState;
    data[1] = g_nvmReadStatus;
    data[2] = g_nvmWriteStatus;
    data[3] = g_nvmValid;
    data[4] = g_batteryCfg.version;
    data[5] = (uint8_t)(g_batteryCfg.save_count & 0xFFU);
    data[6] = (uint8_t)((g_batteryCfg.save_count >> 8U) & 0xFFU);

    (void)NVM_BatteryCfg_SendFrame(NVM_BATTERY_CAN_STATUS_ID, data);
}

static void NVM_BatteryCfg_TxDataIndex(uint8_t index)
{
    uint8_t data[8] = {0U};
    uint16_t can_id = 0U;

    switch (index)
    {
        case 0U:
            U32_ToLe(&data[0], g_batteryCfg.s1_btry1_id);
            U32_ToLe(&data[4], g_batteryCfg.s1_btry2_id);
            can_id = NVM_BATTERY_CAN_DATA1_ID;
            break;

        case 1U:
            U32_ToLe(&data[0], g_batteryCfg.s2_btry1_id);
            U32_ToLe(&data[4], g_batteryCfg.s2_btry2_id);
            can_id = NVM_BATTERY_CAN_DATA2_ID;
            break;

        case 2U:
            U32_ToLe(&data[0], g_batteryCfg.s3_btry1_id);
            U32_ToLe(&data[4], g_batteryCfg.s3_btry2_id);
            can_id = NVM_BATTERY_CAN_DATA3_ID;
            break;

        case 3U:
            U32_ToLe(&data[0], g_batteryCfg.s4_btry1_id);
            U32_ToLe(&data[4], g_batteryCfg.s4_btry2_id);
            can_id = NVM_BATTERY_CAN_DATA4_ID;
            break;

        default:
            return;
    }

    (void)NVM_BatteryCfg_SendFrame(can_id, data);
}

static void NVM_BatteryCfg_TxAck(uint8_t cmd,
                                 uint8_t status,
                                 uint8_t param_id,
                                 uint32_t value)
{
    uint8_t data[8] = {0U};

    data[0] = cmd;
    data[1] = status;
    data[2] = param_id;
    U32_ToLe(&data[4], value);

    (void)NVM_BatteryCfg_SendFrame(NVM_BATTERY_CAN_ACK_ID, data);
}

static void NVM_BatteryCfg_TxAllDataNow(void)
{
    for (uint8_t index = 0U; index < 4U; index++)
    {
        NVM_BatteryCfg_TxDataIndex(index);
    }
}

static void NVM_BatteryCfg_UpdateReadyState(void)
{
    if (g_w25Ready != 0U)
    {
        return;
    }

    W25_Init();
    g_w25JedecId = W25_ReadID();
    g_w25Ready = (g_w25JedecId == W25Q32_JEDEC_ID) ? 1U : 0U;
}

void NVM_BatteryCfg_LoadDefaults(void)
{
    memset(&g_batteryCfg, 0, sizeof(g_batteryCfg));

    g_batteryCfg.magic = NVM_BATTERY_MAGIC;
    g_batteryCfg.version = NVM_BATTERY_VERSION;
    g_batteryCfg.block_id = NVM_BATTERY_BLOCK_ID;
    g_batteryCfg.save_count = 1U;

    g_batteryCfg.s1_btry1_id = 0xFFFFFFFFUL;
    g_batteryCfg.s1_btry2_id = 0xFFFFFFFFUL;
    g_batteryCfg.s2_btry1_id = 0xFFFFFFFFUL;
    g_batteryCfg.s2_btry2_id = 0xFFFFFFFFUL;
    g_batteryCfg.s3_btry1_id = 0xFFFFFFFFUL;
    g_batteryCfg.s3_btry2_id = 0xFFFFFFFFUL;
    g_batteryCfg.s4_btry1_id = 0xFFFFFFFFUL;
    g_batteryCfg.s4_btry2_id = 0xFFFFFFFFUL;

    g_batteryCfg.checksum = NVM_BatteryCfg_CalcChecksum(&g_batteryCfg);
}

uint8_t NVM_BatteryCfg_IsValid(void)
{
    if (g_batteryCfg.magic != NVM_BATTERY_MAGIC)
    {
        return 0U;
    }

    if (g_batteryCfg.version != NVM_BATTERY_VERSION)
    {
        return 0U;
    }

    if (g_batteryCfg.block_id != NVM_BATTERY_BLOCK_ID)
    {
        return 0U;
    }

    if (g_batteryCfg.checksum != NVM_BatteryCfg_CalcChecksum(&g_batteryCfg))
    {
        return 0U;
    }

    return 1U;
}

uint8_t NVM_BatteryCfg_Load(void)
{
    if (g_w25Ready == 0U)
    {
        return 0U;
    }

    W25_ReadData(NVM_W25Q_BASE_ADDRESS,
                 (uint8_t *)&g_batteryCfg,
                 NVM_BATTERY_BLOCK_SIZE_BYTES);
    return 1U;
}

uint8_t NVM_BatteryCfg_Save(void)
{
    NVM_BatteryCfg_t verify;

    if (g_w25Ready == 0U)
    {
        return 0U;
    }

    g_batteryCfg.checksum = NVM_BatteryCfg_CalcChecksum(&g_batteryCfg);

    for (uint8_t attempt = 1U; attempt <= 2U; attempt++)
    {
        W25_SectorErase(NVM_W25Q_BASE_ADDRESS);
        W25_Write(NVM_W25Q_BASE_ADDRESS,
                  (uint8_t *)&g_batteryCfg,
                  NVM_BATTERY_BLOCK_SIZE_BYTES);
        W25_WaitBusy();
        HAL_Delay(1U);

        W25_ReadData(NVM_W25Q_BASE_ADDRESS,
                     (uint8_t *)&verify,
                     sizeof(verify));

        if (memcmp(&g_batteryCfg, &verify, sizeof(g_batteryCfg)) == 0)
        {
            printf("Flash save verified on attempt %u\r\n", attempt);
            return 1U;
        }

        printf("Flash verify failed (attempt %u), status=0x%02X\r\n",
               attempt, W25_ReadStatus());

        for (uint8_t i = 0U; i < sizeof(g_batteryCfg); i++)
        {
            uint8_t expected = ((const uint8_t *)&g_batteryCfg)[i];
            uint8_t actual = ((const uint8_t *)&verify)[i];

            if (expected != actual)
            {
                printf("First mismatch at byte %u: expected 0x%02X, read 0x%02X\r\n",
                       i, expected, actual);
                break;
            }
        }
    }

    return 0U;
}

const NVM_BatteryCfg_t *NVM_BatteryCfg_Get(void)
{
    return &g_batteryCfg;
}

uint32_t NVM_BatteryCfg_GetSlotId(uint8_t slot)
{
    switch (slot)
    {
        case 1U: return g_batteryCfg.s1_btry1_id;
        case 2U: return g_batteryCfg.s1_btry2_id;
        case 3U: return g_batteryCfg.s2_btry1_id;
        case 4U: return g_batteryCfg.s2_btry2_id;
        case 5U: return g_batteryCfg.s3_btry1_id;
        case 6U: return g_batteryCfg.s3_btry2_id;
        case 7U: return g_batteryCfg.s4_btry1_id;
        case 8U: return g_batteryCfg.s4_btry2_id;
        default: return 0U;
    }
}

void NVM_BatteryCfg_Init(void)
{
    g_nvmReadStatus = 0U;
    g_nvmWriteStatus = 0U;
    g_nvmValid = 0U;
    g_txFrameIndex = 0U;
    g_w25JedecId = 0U;
    g_w25Ready = 0U;
    g_batteryState = NVM_BATTERY_STATE_LOAD;
}

uint8_t NVM_BatteryCfg_SetParam(uint8_t param_id, uint32_t value)
{
    if ((value > 0x1FFFFFFFUL) && (value != 0xFFFFFFFFUL))
    {
        return 0U;
    }

    switch (param_id)
    {
        case NVM_PARAM_S1_BTRY1_ID:
            g_batteryCfg.s1_btry1_id = value;
            return 1U;
        case NVM_PARAM_S1_BTRY2_ID:
            g_batteryCfg.s1_btry2_id = value;
            return 1U;
        case NVM_PARAM_S2_BTRY1_ID:
            g_batteryCfg.s2_btry1_id = value;
            return 1U;
        case NVM_PARAM_S2_BTRY2_ID:
            g_batteryCfg.s2_btry2_id = value;
            return 1U;
        case NVM_PARAM_S3_BTRY1_ID:
            g_batteryCfg.s3_btry1_id = value;
            return 1U;
        case NVM_PARAM_S3_BTRY2_ID:
            g_batteryCfg.s3_btry2_id = value;
            return 1U;
        case NVM_PARAM_S4_BTRY1_ID:
            g_batteryCfg.s4_btry1_id = value;
            return 1U;
        case NVM_PARAM_S4_BTRY2_ID:
            g_batteryCfg.s4_btry2_id = value;
            return 1U;
        default:
            return 0U;
    }
}

void NVM_BatteryCfg_SaveCommand(void)
{
    g_batteryState = NVM_BATTERY_STATE_SAVE_CMD;
}

void NVM_BatteryCfg_CanRx(uint32_t can_id, const uint8_t *data, uint8_t dlc)
{
    uint8_t param_id;
    uint32_t value;
    uint8_t ok;
    uint8_t data_index;

    if ((data == NULL) || (dlc < 8U))
    {
        return;
    }

    if (can_id == NVM_BATTERY_CAN_RX_SET_ID)
    {
        param_id = data[0];
        value = Le_ToU32(&data[2]);
        ok = NVM_BatteryCfg_SetParam(param_id, value);

        if (ok != 0U)
        {
            NVM_BatteryCfg_TxAck(0x01U, 0x00U, param_id, value);

            data_index = (uint8_t)((param_id - 1U) / 2U);
            if (data_index < 4U)
            {
                NVM_BatteryCfg_TxDataIndex(data_index);
            }
        }
        else
        {
            NVM_BatteryCfg_TxAck(0x01U, 0x01U, param_id, value);
        }
    }
    else if (can_id == NVM_BATTERY_CAN_RX_SAVE_ID)
    {
        if (data[0] == 0xA5U)
        {
            NVM_BatteryCfg_SaveCommand();
            NVM_BatteryCfg_TxAck(0x02U, 0x00U, 0U, 0U);
            NVM_BatteryCfg_TxStatus();
            NVM_BatteryCfg_TxAllDataNow();
        }
        else
        {
            NVM_BatteryCfg_TxAck(0x02U, 0x01U, 0U, 0U);
        }
    }
}

void NVM_BatteryCfg_Task(void)
{
    switch (g_batteryState)
    {
        case NVM_BATTERY_STATE_IDLE:
        case NVM_BATTERY_STATE_DONE:
            break;

        case NVM_BATTERY_STATE_LOAD:
            NVM_BatteryCfg_UpdateReadyState();
            if (g_w25Ready != 0U)
            {
                g_nvmReadStatus = NVM_BatteryCfg_Load();
            }
            else
            {
                g_nvmReadStatus = 0U;
            }
            g_batteryState = NVM_BATTERY_STATE_VALIDATE;
            break;

        case NVM_BATTERY_STATE_VALIDATE:
            g_nvmValid = NVM_BatteryCfg_IsValid();
            if (g_nvmValid == 0U)
            {
                g_batteryState = NVM_BATTERY_STATE_DEFAULT_SAVE;
            }
            else
            {
                g_batteryState = NVM_BATTERY_STATE_TX_STATUS;
            }
            break;

        case NVM_BATTERY_STATE_DEFAULT_SAVE:
            NVM_BatteryCfg_LoadDefaults();
            if (g_w25Ready != 0U)
            {
                g_nvmWriteStatus = NVM_BatteryCfg_Save();
                g_batteryState = NVM_BATTERY_STATE_LOAD;
            }
            else
            {
                g_nvmWriteStatus = 0U;
                g_batteryState = NVM_BATTERY_STATE_TX_STATUS;
            }
            break;

        case NVM_BATTERY_STATE_TX_STATUS:
            NVM_BatteryCfg_TxStatus();
            g_txFrameIndex = 0U;
            g_batteryState = NVM_BATTERY_STATE_TX_DATA;
            break;

        case NVM_BATTERY_STATE_TX_DATA:
            NVM_BatteryCfg_TxDataIndex(g_txFrameIndex);
            g_txFrameIndex++;
            if (g_txFrameIndex >= 4U)
            {
                g_batteryState = NVM_BATTERY_STATE_DONE;
            }
            break;

        case NVM_BATTERY_STATE_SAVE_CMD:
            g_batteryCfg.save_count++;
            g_nvmWriteStatus = NVM_BatteryCfg_Save();
            g_batteryState = NVM_BATTERY_STATE_LOAD;
            break;

        default:
            g_batteryState = NVM_BATTERY_STATE_DONE;
            break;
    }
}
