#include "can_service.h"
#include "battery_flash.h"
#include "w25q32.h"
#include <string.h>

#define FLASH_TEST_ADDRESS  0x001000UL
#define FLASH_TEST_PASS     0xA5U
#define FLASH_TEST_ERASE    0xE1U
#define FLASH_TEST_WRITE    0xE2U

HAL_StatusTypeDef CAN_Service_Init(void)
{
    return MCP2515_Init();
}

static void CAN_Service_Process(const MCP2515_Frame_t *frame)
{
    uint8_t batteryNumber;
    uint32_t batteryID;

    if ((frame->id == CAN_ID_BATTERY_WRITE) && (frame->dlc >= 6U))
    {
        batteryNumber = frame->data[0];
        batteryID = ((uint32_t)frame->data[5] << 24) |
                    ((uint32_t)frame->data[4] << 16) |
                    ((uint32_t)frame->data[3] << 8)  |
                     (uint32_t)frame->data[2];

        (void)BatteryFlash_SetID(batteryNumber, batteryID);
    }
    else if ((frame->id == CAN_ID_BATTERY_SAVE) &&
             (frame->dlc >= 1U) &&
             (frame->data[0] == BATTERY_SAVE_CMD))
    {
        HAL_StatusTypeDef saveStatus = BatteryFlash_Save();

        if (saveStatus == HAL_OK)
        {
            (void)CAN_Service_SendBatteryIDs();
            (void)CAN_Service_SendSaveStatus(BATTERY_SAVE_CMD);
        }
        else
        {
            (void)CAN_Service_SendSaveStatus(0U);
        }
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
}

void CAN_Service_Poll(void)
{
    MCP2515_Frame_t frame;

    if (MCP2515_Receive(&frame) == HAL_OK)
    {
        CAN_Service_Process(&frame);
    }
}

HAL_StatusTypeDef CAN_Service_SendBatteryIDs(void)
{
    MCP2515_Frame_t frame = {0};

    frame.dlc = 8U;

    for (uint8_t index = 0U; index < 4U; index++)
    {
        uint32_t id1 = BatteryFlash_GetID((uint8_t)(index * 2U + 1U));
        uint32_t id2 = BatteryFlash_GetID((uint8_t)(index * 2U + 2U));

        frame.id = CAN_ID_BATTERY_TX1 + index;
        frame.data[0] = (uint8_t)id1;
        frame.data[1] = (uint8_t)(id1 >> 8);
        frame.data[2] = (uint8_t)(id1 >> 16);
        frame.data[3] = (uint8_t)(id1 >> 24);
        frame.data[4] = (uint8_t)id2;
        frame.data[5] = (uint8_t)(id2 >> 8);
        frame.data[6] = (uint8_t)(id2 >> 16);
        frame.data[7] = (uint8_t)(id2 >> 24);

        if (MCP2515_Send(&frame) != HAL_OK)
        {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

HAL_StatusTypeDef CAN_Service_SendSaveStatus(uint8_t status)
{
    MCP2515_Frame_t frame = {0};

    frame.id = CAN_ID_SAVE_STATUS;
    frame.dlc = 1U;
    frame.data[0] = status;
    return MCP2515_Send(&frame);
}
