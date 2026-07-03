#include "battery_can_rx.h"
#include "nvm_config.h"
#include "vehicle_state.h"
#include <string.h>

#define BATTERY_RX_TIMEOUT_MS 500U

static uint32_t lastTaskTick;

static uint16_t U16_MsbLsb(uint8_t msb, uint8_t lsb)
{
    return (uint16_t)(((uint16_t)msb << 8) | lsb);
}

static void CopyAscii7(char *destination, uint8_t size, const uint8_t *data)
{
    uint8_t count = (size > 0U) ? (uint8_t)(size - 1U) : 0U;

    if (count > 7U)
    {
        count = 7U;
    }

    for (uint8_t i = 0U; i < count; i++)
    {
        destination[i] = (char)data[i + 1U];
    }

    if (size > 0U)
    {
        destination[count] = '\0';
    }
}

static void DecodeFrame(BatteryState_t *battery, const uint8_t *data)
{
    switch (data[0])
    {
        case 0x00U:
            memcpy(battery->BmsUniqueIdRaw, data, 8U);
            break;

        case 0x01U:
            battery->BatteryVoltage_raw = U16_MsbLsb(data[1], data[2]);
            battery->Voltage_dV = battery->BatteryVoltage_raw / 10U;
            battery->BatteryCurrent_raw = U16_MsbLsb(data[3], data[4]);
            battery->Current_dA = (int16_t)(battery->BatteryCurrent_raw / 10U);
            battery->BatteryTemp_C = (int8_t)data[5];
            battery->LoadStatus = data[6];
            battery->Status = data[7];
            break;

        case 0x02U:
            battery->SOH = data[1];
            battery->SOC = data[2];
            battery->Cycles = U16_MsbLsb(data[3], data[4]);
            battery->BatteryCapacity_raw = U16_MsbLsb(data[5], data[6]);
            battery->Capacity = battery->BatteryCapacity_raw;
            battery->Charge = data[7];
            break;

        case 0x03U:
            battery->Switch_Status = data[1];
            battery->CellThermistor_C[9] = (int8_t)data[5];
            battery->CellThermistor_C[8] = (int8_t)data[6];
            battery->CellThermistor_C[7] = (int8_t)data[7];
            break;

        case 0x04U:
            for (uint8_t i = 0U; i < 7U; i++)
            {
                battery->CellThermistor_C[i] = (int8_t)data[i + 1U];
            }
            break;

        case 0x05U:
            CopyAscii7(battery->ProtocolVersion,
                       sizeof(battery->ProtocolVersion), data);
            break;

        case 0x06U:
            CopyAscii7(battery->BatterySerialNumber,
                       sizeof(battery->BatterySerialNumber), data);
            break;

        case 0x07U:
            battery->Ambient_Temp_C = (int8_t)data[1];
            battery->MOSFET_Temp_C = (int8_t)data[2];
            break;

        case 0x08U:
            battery->AlarmBits_1 = data[1];
            battery->AlarmBits_2 = data[2];
            battery->ProtBits_1 = data[3];
            battery->ProtBits_2 = data[4];
            battery->FaultBits_1 = data[5];
            break;

        case 0x09U:
        case 0x0AU:
        case 0x0BU:
        case 0x0CU:
        case 0x0DU:
        {
            uint8_t firstCell = (uint8_t)((data[0] - 0x09U) * 3U);
            battery->CellVoltage_raw_mV[firstCell] =
                U16_MsbLsb(data[1], data[2]);
            battery->CellVoltage_raw_mV[firstCell + 1U] =
                U16_MsbLsb(data[3], data[4]);
            battery->CellVoltage_raw_mV[firstCell + 2U] =
                U16_MsbLsb(data[5], data[6]);
            break;
        }

        case 0x0EU:
            battery->CellVoltage_raw_mV[15] =
                U16_MsbLsb(data[1], data[2]);
            break;

        default:
            break;
    }
}

void BatteryCAN_Init(void)
{
    memset(&VS_Battery1, 0, sizeof(VS_Battery1));
    memset(&VS_Battery2, 0, sizeof(VS_Battery2));
    lastTaskTick = HAL_GetTick();
}

void BatteryCAN_ProcessFrame(const MCP2515_Frame_t *frame)
{
    BatteryState_t *battery = NULL;

    if ((frame == NULL) || (frame->is_extended == 0U) ||
        (frame->dlc < 8U))
    {
        return;
    }

    for (uint8_t slot = 1U; slot <= MAX_BATTERIES; slot++)
    {
        if (frame->id == NVM_BatteryCfg_GetSlotId(slot))
        {
            battery = ((slot & 1U) != 0U) ?
                      &VS_Battery1 : &VS_Battery2;
            break;
        }
    }

    if (battery == NULL)
    {
        return;
    }

    DecodeFrame(battery, frame->data);
    battery->Alive = 1U;
    battery->RxTimeoutMs = BATTERY_RX_TIMEOUT_MS;
}

static void UpdateTimeout(BatteryState_t *battery, uint32_t elapsedMs)
{
    if (battery->RxTimeoutMs > elapsedMs)
    {
        battery->RxTimeoutMs = (uint16_t)(battery->RxTimeoutMs - elapsedMs);
    }
    else
    {
        memset(battery, 0, sizeof(*battery));
    }
}

void BatteryCAN_Task(void)
{
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - lastTaskTick;

    if (elapsed == 0U)
    {
        return;
    }

    lastTaskTick = now;
    UpdateTimeout(&VS_Battery1, elapsed);
    UpdateTimeout(&VS_Battery2, elapsed);
}
