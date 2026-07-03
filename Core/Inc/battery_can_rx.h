#ifndef BATTERY_CAN_RX_H
#define BATTERY_CAN_RX_H

#include "mcp2515.h"

void BatteryCAN_Init(void);
void BatteryCAN_ProcessFrame(const MCP2515_Frame_t *frame);
void BatteryCAN_Task(void);

#endif
