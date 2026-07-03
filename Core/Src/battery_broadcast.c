#include "battery_broadcast.h"
#include "mcp2515.h"
#include "vehicle_state.h"
#include <string.h>

#define CAN_ID_BATTERY1_FAST  0x705U
#define CAN_ID_BATTERY2_FAST  0x706U
#define CAN_ID_BATTERY1_WVC   0x707U
#define CAN_ID_BATTERY2_WVC   0x708U
#define CAN_ID_BATTERY1_TEMP  0x721U
#define CAN_ID_BATTERY1_CELL1 0x722U
#define CAN_ID_BATTERY1_CELL2 0x723U
#define CAN_ID_BATTERY1_CELL3 0x724U
#define CAN_ID_BATTERY1_CELL4 0x725U
#define CAN_ID_BATTERY2_TEMP  0x726U
#define CAN_ID_BATTERY2_CELL1 0x727U
#define CAN_ID_BATTERY2_CELL2 0x728U
#define CAN_ID_BATTERY2_CELL3 0x729U
#define CAN_ID_BATTERY2_CELL4 0x72AU
#define CAN_ID_BATTERY1_START 0x740U
#define CAN_ID_BATTERY2_START 0x741U

#define BROADCAST_1S_TICKS           10U
#define BROADCAST_60S_TICKS          600U
#define STARTUP_FRAME_TX_LIMIT       20U

static uint16_t broadcast1sCounter;
static uint16_t broadcast60sCounter;
static uint16_t startupCounter;
static uint8_t startupDone;
static uint8_t startupFrameTxCount;
static uint8_t broadcast60sPending;

static HAL_StatusTypeDef SendFrame(uint16_t canId, const uint8_t *data)
{
    MCP2515_Frame_t frame = {0};

    frame.id = canId;
    frame.is_extended = 0U;
    frame.dlc = 8U;

    for (uint8_t i = 0U; i < 8U; i++)
    {
        frame.data[i] = data[i];
    }

    return MCP2515_Send(&frame);
}

static uint8_t ClampTemp(int8_t value)
{
    if (value < 0)
    {
        return 0U;
    }

    return (uint8_t)value;
}

static HAL_StatusTypeDef SendFastFrame(uint16_t canId,
                                       const BatteryState_t *battery)
{
    uint8_t data[8] = {0U};

    data[0] = (uint8_t)(battery->BatteryCurrent_raw >> 8);
    data[1] = (uint8_t)battery->BatteryCurrent_raw;
    data[2] = (uint8_t)(battery->BatteryVoltage_raw >> 8);
    data[3] = (uint8_t)battery->BatteryVoltage_raw;
    data[4] = battery->LoadStatus;
    data[5] = battery->Alive;

    return SendFrame(canId, data);
}

static HAL_StatusTypeDef SendWvcFrame(uint16_t canId,
                                      const BatteryState_t *battery)
{
    uint8_t data[8] = {0U};

    data[0] = battery->SOH;
    data[1] = (uint8_t)(battery->Cycles >> 8);
    data[2] = (uint8_t)battery->Cycles;
    data[3] = (uint8_t)(battery->BatteryCapacity_raw >> 8);
    data[4] = (uint8_t)battery->BatteryCapacity_raw;

    return SendFrame(canId, data);
}

static HAL_StatusTypeDef SendTemperatureFrame(uint16_t canId,
                                              const BatteryState_t *battery)
{
    uint8_t data[8] = {0U};

    data[0] = battery->SOC;
    data[1] = ClampTemp(battery->MOSFET_Temp_C);
    data[2] = ClampTemp(battery->BatteryTemp_C);
    data[3] = ClampTemp(battery->CellThermistor_C[0]);
    data[4] = ClampTemp(battery->CellThermistor_C[1]);
    data[5] = ClampTemp(battery->CellThermistor_C[2]);
    data[6] = ClampTemp(battery->CellThermistor_C[3]);

    return SendFrame(canId, data);
}

static HAL_StatusTypeDef SendCellVoltageFrame(uint16_t canId,
                                              const BatteryState_t *battery,
                                              uint8_t firstCell)
{
    uint8_t data[8];

    for (uint8_t i = 0U; i < 4U; i++)
    {
        uint16_t cell = battery->CellVoltage_raw_mV[firstCell + i];
        uint8_t index = (uint8_t)(i * 2U);

        data[index] = (uint8_t)(cell >> 8);
        data[index + 1U] = (uint8_t)cell;
    }

    return SendFrame(canId, data);
}

static HAL_StatusTypeDef SendStartupFrame(uint16_t canId,
                                          const BatteryState_t *battery)
{
    uint8_t data[8] = {0U};

    memcpy(&data[0], battery->BatterySerialNumber, 7U);
    data[7] = battery->Switch_Status;
    return SendFrame(canId, data);
}

static void BatteryBroadcast_Send1sFrames(void)
{
    (void)SendFastFrame(CAN_ID_BATTERY1_FAST, &VS_Battery1);
    (void)SendFastFrame(CAN_ID_BATTERY2_FAST, &VS_Battery2);
}

static void BatteryBroadcast_Send60sFrames(void)
{
    (void)SendWvcFrame(CAN_ID_BATTERY1_WVC, &VS_Battery1);
    (void)SendWvcFrame(CAN_ID_BATTERY2_WVC, &VS_Battery2);
    (void)SendTemperatureFrame(CAN_ID_BATTERY1_TEMP, &VS_Battery1);
    (void)SendTemperatureFrame(CAN_ID_BATTERY2_TEMP, &VS_Battery2);
    (void)SendCellVoltageFrame(CAN_ID_BATTERY1_CELL1, &VS_Battery1, 0U);
    (void)SendCellVoltageFrame(CAN_ID_BATTERY1_CELL2, &VS_Battery1, 4U);
    (void)SendCellVoltageFrame(CAN_ID_BATTERY1_CELL3, &VS_Battery1, 8U);
    (void)SendCellVoltageFrame(CAN_ID_BATTERY1_CELL4, &VS_Battery1, 12U);
    (void)SendCellVoltageFrame(CAN_ID_BATTERY2_CELL1, &VS_Battery2, 0U);
    (void)SendCellVoltageFrame(CAN_ID_BATTERY2_CELL2, &VS_Battery2, 4U);
    (void)SendCellVoltageFrame(CAN_ID_BATTERY2_CELL3, &VS_Battery2, 8U);
    (void)SendCellVoltageFrame(CAN_ID_BATTERY2_CELL4, &VS_Battery2, 12U);
}

static void BatteryBroadcast_SendStartupFrames(void)
{
    (void)SendStartupFrame(CAN_ID_BATTERY1_START, &VS_Battery1);
    (void)SendStartupFrame(CAN_ID_BATTERY2_START, &VS_Battery2);
}

void BatteryBroadcast_Init(void)
{
    broadcast1sCounter = 0U;
    broadcast60sCounter = 0U;
    startupDone = 0U;
    broadcast60sPending = 0U;
    startupFrameTxCount = 0U;
    startupCounter = BROADCAST_60S_TICKS;
}

void BatteryBroadcast_Task(void)
{
    if (startupDone == 0U)
    {
        if (startupCounter > 0U)
        {
            startupCounter--;
        }

        if (startupCounter == 0U)
        {
            BatteryBroadcast_SendStartupFrames();
            startupFrameTxCount++;

            if (startupFrameTxCount >= STARTUP_FRAME_TX_LIMIT)
            {
                startupDone = 1U;
            }
            else
            {
                startupCounter = BROADCAST_60S_TICKS;
            }

            return;
        }
    }

    if (broadcast60sPending != 0U)
    {
        BatteryBroadcast_Send60sFrames();
        broadcast60sPending = 0U;
        return;
    }

    broadcast1sCounter++;
    broadcast60sCounter++;

    if (broadcast1sCounter >= BROADCAST_1S_TICKS)
    {
        broadcast1sCounter = 0U;
        BatteryBroadcast_Send1sFrames();
    }

    if (broadcast60sCounter >= BROADCAST_60S_TICKS)
    {
        broadcast60sCounter = 0U;
        broadcast60sPending = 1U;
    }
}
