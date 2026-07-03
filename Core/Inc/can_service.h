#ifndef CAN_SERVICE_H
#define CAN_SERVICE_H

#include "mcp2515.h"
#include <stdint.h>

/* Flash diagnostic: send 0xA5 on 0x540; result is returned on 0x541. */
#define CAN_ID_FLASH_TEST        0x540
#define CAN_ID_FLASH_TEST_RESULT 0x541

/* RAM diagnostic: 0x550 D0=1/2; response is 0x551/0x552. */
#define CAN_ID_BATTERY_DIAG_REQUEST  0x550
#define CAN_ID_BATTERY1_DIAG_RESULT  0x551
#define CAN_ID_BATTERY2_DIAG_RESULT  0x552

/* Initialize CAN */
HAL_StatusTypeDef CAN_Service_Init(void);

/* Poll and process one received CAN frame. */
void CAN_Service_Poll(void);

#endif
