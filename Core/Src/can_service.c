#include "can_service.h"
#include "battery_can_rx.h"
#include "nvm_config.h"
#include "vehicle_state.h"
#include "w25q32.h"
#include <string.h>

#define FLASH_TEST_ADDRESS  0x001000UL
#define FLASH_TEST_PASS     0xA5U
#define FLASH_TEST_ERASE    0xE1U
#define FLASH_TEST_WRITE    0xE2U

static HAL_StatusTypeDef CAN_Service_SendBatteryDiagnostic(uint8_t number)
{
    const BatteryState_t *battery;
    MCP2515_Frame_t result = {0};

    if (number == 1U)
    {
        battery = &VS_Battery1;
        result.id = CAN_ID_BATTERY1_DIAG_RESULT;
    }
    else if (number == 2U)
    {
        battery = &VS_Battery2;
        result.id = CAN_ID_BATTERY2_DIAG_RESULT;
    }
    else
    {
        return HAL_ERROR;
    }

    result.dlc = 8U;
    result.data[0] = (uint8_t)(battery->BatteryVoltage_raw >> 8);
    result.data[1] = (uint8_t)battery->BatteryVoltage_raw;
    result.data[2] = (uint8_t)(battery->BatteryCurrent_raw >> 8);
    result.data[3] = (uint8_t)battery->BatteryCurrent_raw;
    result.data[4] = (uint8_t)battery->BatteryTemp_C;
    result.data[5] = battery->LoadStatus;
    result.data[6] = battery->Status;
    result.data[7] = battery->Alive;

    return MCP2515_Send(&result);
}

HAL_StatusTypeDef CAN_Service_Init(void)
{
    return MCP2515_Init();
}

static void CAN_Service_Process(const MCP2515_Frame_t *frame)
{
    if (frame->is_extended != 0U)
    {
        BatteryCAN_ProcessFrame(frame);
        return;
    }

    if ((frame->id == NVM_BATTERY_CAN_RX_SET_ID) ||
        (frame->id == NVM_BATTERY_CAN_RX_SAVE_ID))
    {
        NVM_BatteryCfg_CanRx(frame->id, frame->data, frame->dlc);
    }
    else if ((frame->id == CAN_ID_FLASH_TEST) &&
             (frame->dlc >= 1U) &&
             (frame->data[0] == 0xA5U))
    {
        static const uint8_t testPattern[4] = {0x12U, 0x34U, 0x56U, 0x78U};
        uint8_t readback[4] = {0U};
        MCP2515_Frame_t result = {0};

        result.id = CAN_ID_FLASH_TEST_RESULT;
        result.dlc = 5U;

        W25_SectorErase(FLASH_TEST_ADDRESS);
        W25_ReadData(FLASH_TEST_ADDRESS, readback, sizeof(readback));

        if ((readback[0] != 0xFFU) || (readback[1] != 0xFFU) ||
            (readback[2] != 0xFFU) || (readback[3] != 0xFFU))
        {
            result.data[0] = FLASH_TEST_ERASE;
        }
        else
        {
            W25_Write(FLASH_TEST_ADDRESS, (uint8_t *)testPattern,
                      sizeof(testPattern));
            W25_ReadData(FLASH_TEST_ADDRESS, readback, sizeof(readback));

            result.data[0] =
                (memcmp(testPattern, readback, sizeof(testPattern)) == 0) ?
                FLASH_TEST_PASS : FLASH_TEST_WRITE;
        }

        memcpy(&result.data[1], readback, sizeof(readback));
        (void)MCP2515_Send(&result);
    }
    else if ((frame->id == CAN_ID_BATTERY_DIAG_REQUEST) &&
             (frame->dlc >= 1U))
    {
        (void)CAN_Service_SendBatteryDiagnostic(frame->data[0]);
    }
}

void CAN_Service_Poll(void)
{
    MCP2515_Frame_t frame;

    if (MCP2515_Receive(&frame) == HAL_OK)
    {
        CAN_Service_Process(&frame);
    }
}
