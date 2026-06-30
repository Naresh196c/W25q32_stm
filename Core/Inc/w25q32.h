#ifndef __W25Q32_H
#define __W25Q32_H

#include "main.h"

/* SPI Handle */
#define W25_CMD_READ_DATA    0x03
#define W25_CMD_SECTOR_ERASE    0x20
#define W25_CMD_PAGE_PROGRAM    0x02

void W25_PageProgram(uint32_t address,
                     uint8_t *buffer,
                     uint16_t length);

void W25_Write(uint32_t address,
               uint8_t *buffer,
               uint32_t length);

void W25_ReadData(uint32_t address, uint8_t *buffer, uint16_t length);

extern SPI_HandleTypeDef hspi2;


void W25_WaitBusy(void);
void W25_SectorErase(uint32_t address);
/* Chip Select Pin */
#define W25_CS_PORT    GPIOA
#define W25_CS_PIN     GPIO_PIN_8

#define W25_CS_LOW()   HAL_GPIO_WritePin(W25_CS_PORT, W25_CS_PIN, GPIO_PIN_RESET)
#define W25_CS_HIGH()  HAL_GPIO_WritePin(W25_CS_PORT, W25_CS_PIN, GPIO_PIN_SET)

/* Command */
#define W25_CMD_JEDEC_ID    0x9F

void W25_Init(void);
uint8_t W25_ReadStatus(void);
void W25_WriteEnable(void);
uint32_t W25_ReadID(void);

#endif
