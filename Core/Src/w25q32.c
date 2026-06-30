#include "w25q32.h"
#define W25_CMD_READ_STATUS1    0x05
#define W25_CMD_WRITE_ENABLE    0x06

void W25_WriteEnable(void)
{
    uint8_t cmd = W25_CMD_WRITE_ENABLE;

    W25_WaitBusy();

    W25_CS_LOW();

    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);

    W25_CS_HIGH();
}
uint8_t W25_ReadStatus(void)
{
    uint8_t cmd = W25_CMD_READ_STATUS1;
    uint8_t status = 0;

    W25_CS_LOW();

    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, &status, 1, HAL_MAX_DELAY);

    W25_CS_HIGH();

    return status;
}
static void W25_Select(void)
{
    W25_CS_LOW();
}

static void W25_Unselect(void)
{
    W25_CS_HIGH();
}

void W25_Init(void)
{
    W25_Unselect();
}

uint32_t W25_ReadID(void)
{
    uint8_t cmd = W25_CMD_JEDEC_ID;
    uint8_t id[3] = {0};

    W25_Select();

    HAL_SPI_Transmit(&hspi2, &cmd, 1, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, id, 3, HAL_MAX_DELAY);

    W25_Unselect();

    return ((uint32_t)id[0] << 16) |
           ((uint32_t)id[1] << 8)  |
            id[2];
}

void W25_WaitBusy(void)
{
    while (W25_ReadStatus() & 0x01);
}

void W25_SectorErase(uint32_t address)
{
    uint8_t cmd[4];

    W25_WriteEnable();

    /* WEL (bit 1) must be set before an erase command is accepted. */
    if ((W25_ReadStatus() & 0x02U) == 0U)
    {
        return;
    }

    cmd[0] = W25_CMD_SECTOR_ERASE;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    W25_CS_LOW();

    HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);

    W25_CS_HIGH();

    W25_WaitBusy();
}

void W25_ReadData(uint32_t address, uint8_t *buffer, uint16_t length)
{
    uint8_t cmd[4];

    cmd[0] = W25_CMD_READ_DATA;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    W25_CS_LOW();

    HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);
    HAL_SPI_Receive(&hspi2, buffer, length, HAL_MAX_DELAY);

    W25_CS_HIGH();
}

void W25_PageProgram(uint32_t address,
                     uint8_t *buffer,
                     uint16_t length)
{
    uint8_t cmd[4];

    W25_WriteEnable();

    /* WEL (bit 1) must be set before a program command is accepted. */
    if ((W25_ReadStatus() & 0x02U) == 0U)
    {
        return;
    }

    cmd[0] = W25_CMD_PAGE_PROGRAM;
    cmd[1] = (address >> 16) & 0xFF;
    cmd[2] = (address >> 8) & 0xFF;
    cmd[3] = address & 0xFF;

    W25_CS_LOW();

    HAL_SPI_Transmit(&hspi2, cmd, 4, HAL_MAX_DELAY);

    HAL_SPI_Transmit(&hspi2,
                     buffer,
                     length,
                     HAL_MAX_DELAY);

    W25_CS_HIGH();

    W25_WaitBusy();
}

void W25_Write(uint32_t address,
               uint8_t *buffer,
               uint32_t length)
{
    uint16_t bytes_to_write;
    uint16_t page_offset;

    while(length > 0)
    {
        /* Current offset inside the page */
        page_offset = address % 256;

        /* Remaining space in this page */
        bytes_to_write = 256 - page_offset;

        /* Don't exceed requested length */
        if(bytes_to_write > length)
        {
            bytes_to_write = length;
        }

        /* Write one page */
        W25_PageProgram(address,
                        buffer,
                        bytes_to_write);

        /* Advance */
        address += bytes_to_write;
        buffer += bytes_to_write;
        length -= bytes_to_write;
    }
}
