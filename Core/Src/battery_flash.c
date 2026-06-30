#include "battery_flash.h"
#include "w25q32.h"
#include <stdio.h>
#include <string.h>

/* Global RAM Copy */
FlashBattery_t FlashBattery;

/* Initialize Module */
void BatteryFlash_Init(void)
{
    BatteryFlash_Load();
}

/* Load Complete Structure From Flash */
void BatteryFlash_Load(void)
{
    W25_ReadData(BATTERY_FLASH_ADDR,
                 (uint8_t *)&FlashBattery,
                 sizeof(FlashBattery));

    if(FlashBattery.Magic != BATTERY_DATA_MAGIC)
    {
        BatteryFlash_Clear();
    }
}

/* Save Complete Structure To Flash */
HAL_StatusTypeDef BatteryFlash_Save(void)
{
    FlashBattery_t verify;

    FlashBattery.Magic = BATTERY_DATA_MAGIC;

    for (uint8_t attempt = 1U; attempt <= 2U; attempt++)
    {
        W25_SectorErase(BATTERY_FLASH_ADDR);
        W25_Write(BATTERY_FLASH_ADDR,
                  (uint8_t *)&FlashBattery,
                  sizeof(FlashBattery));
        W25_WaitBusy();
        HAL_Delay(1U);
        W25_ReadData(BATTERY_FLASH_ADDR,
                     (uint8_t *)&verify,
                     sizeof(verify));

        if (memcmp(&FlashBattery, &verify, sizeof(FlashBattery)) == 0)
        {
            printf("Flash save verified on attempt %u\r\n", attempt);
            return HAL_OK;
        }

        printf("Flash verify failed (attempt %u), status=0x%02X\r\n",
               attempt, W25_ReadStatus());

        for (uint8_t i = 0U; i < sizeof(FlashBattery); i++)
        {
            uint8_t expected = ((uint8_t *)&FlashBattery)[i];
            uint8_t actual = ((uint8_t *)&verify)[i];

            if (expected != actual)
            {
                printf("First mismatch at byte %u: expected 0x%02X, read 0x%02X\r\n",
                       i, expected, actual);
                break;
            }
        }
    }

    return HAL_ERROR;
}

/* Update One Battery ID */
HAL_StatusTypeDef BatteryFlash_SetID(uint8_t battery_number,
                                     uint32_t battery_id)
{
    if((battery_number < 1) || (battery_number > MAX_BATTERIES))
    {
        return HAL_ERROR;
    }

    FlashBattery.BatteryID[battery_number - 1] = battery_id;

    return HAL_OK;
}

/* Read One Battery ID */
uint32_t BatteryFlash_GetID(uint8_t battery_number)
{
    if((battery_number < 1) || (battery_number > MAX_BATTERIES))
    {
        return 0;
    }

    return FlashBattery.BatteryID[battery_number - 1];
}

/* Clear All Battery IDs */
void BatteryFlash_Clear(void)
{
    FlashBattery.Magic = BATTERY_DATA_MAGIC;
    for(uint8_t i = 0U; i < MAX_BATTERIES; i++)
    {
        FlashBattery.BatteryID[i] = 0xFFFFFFFFUL;
    }
}
