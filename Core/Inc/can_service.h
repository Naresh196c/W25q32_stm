#ifndef CAN_SERVICE_H
#define CAN_SERVICE_H

#include "mcp2515.h"
#include <stdint.h>

/* Receive CAN IDs */
/* 0x530, DLC >= 6: byte 0=slot (1..8), byte 1=reserved,
 * bytes 2..5=battery ID, little endian. */
#define CAN_ID_BATTERY_WRITE     0x530
/* 0x531, DLC >= 1: byte 0=0xA5 saves RAM IDs to W25Q32. */
#define CAN_ID_BATTERY_SAVE      0x531

/* Flash diagnostic: send 0xA5 on 0x540; result is returned on 0x541. */
#define CAN_ID_FLASH_TEST        0x540
#define CAN_ID_FLASH_TEST_RESULT 0x541

/* Transmit CAN IDs */
/* 0x521..0x524: two little-endian uint32 battery IDs per frame. */
#define CAN_ID_BATTERY_TX1       0x521
#define CAN_ID_BATTERY_TX2       0x522
#define CAN_ID_BATTERY_TX3       0x523
#define CAN_ID_BATTERY_TX4       0x524
/* 0x525: byte 0=0xA5 on success, 0x00 on failure. */
#define CAN_ID_SAVE_STATUS       0x525

/* Save Command */
#define BATTERY_SAVE_CMD         0xA5

/* Initialize CAN */
HAL_StatusTypeDef CAN_Service_Init(void);

/* Poll and process one received CAN frame. */
void CAN_Service_Poll(void);

/* Send Stored Battery IDs */
HAL_StatusTypeDef CAN_Service_SendBatteryIDs(void);

/* Send Save Status */
HAL_StatusTypeDef CAN_Service_SendSaveStatus(uint8_t status);

#endif
