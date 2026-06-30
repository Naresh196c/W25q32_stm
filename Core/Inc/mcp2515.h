#ifndef MCP2515_H
#define MCP2515_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

#define MCP2515_CS_PORT GPIOB
#define MCP2515_CS_PIN  GPIO_PIN_0

typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
} MCP2515_Frame_t;

HAL_StatusTypeDef MCP2515_Init(void);
HAL_StatusTypeDef MCP2515_Send(const MCP2515_Frame_t *frame);
HAL_StatusTypeDef MCP2515_Receive(MCP2515_Frame_t *frame);

#endif
