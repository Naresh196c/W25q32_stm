#ifndef BATTERY_FLASH_H
#define BATTERY_FLASH_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

/* Number of Batteries */
#define MAX_BATTERIES      8

/* Flash Storage Address */
#define BATTERY_FLASH_ADDR 0x000000
#define BATTERY_DATA_MAGIC 0x42415454UL

typedef struct
{
    uint32_t Magic;
    uint32_t BatteryID[MAX_BATTERIES];
} FlashBattery_t;

/* Global RAM Copy */
extern FlashBattery_t FlashBattery;

/* Initialize Module */
void BatteryFlash_Init(void);

/* Read Entire Structure From Flash */
void BatteryFlash_Load(void);

/* Save Entire Structure To Flash */
HAL_StatusTypeDef BatteryFlash_Save(void);

/* Update One Battery ID in RAM */
HAL_StatusTypeDef BatteryFlash_SetID(uint8_t battery_number,
                                     uint32_t battery_id);

/* Read One Battery ID From RAM */
uint32_t BatteryFlash_GetID(uint8_t battery_number);

/* Clear All Battery IDs */
void BatteryFlash_Clear(void);

#endif
