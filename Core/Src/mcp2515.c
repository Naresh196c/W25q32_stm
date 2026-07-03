#include "mcp2515.h"

extern SPI_HandleTypeDef hspi1;

#define MCP_CMD_RESET          0xC0U
#define MCP_CMD_READ           0x03U
#define MCP_CMD_WRITE          0x02U
#define MCP_CMD_BIT_MODIFY     0x05U
#define MCP_CMD_RTS_TX0        0x81U

#define MCP_CANSTAT            0x0EU
#define MCP_CANCTRL            0x0FU
#define MCP_CNF3               0x28U
#define MCP_CNF2               0x29U
#define MCP_CNF1               0x2AU
#define MCP_CANINTE            0x2BU
#define MCP_CANINTF            0x2CU
#define MCP_EFLG               0x2DU
#define MCP_TXB0CTRL           0x30U
#define MCP_TXB0SIDH           0x31U
#define MCP_RXB0CTRL           0x60U
#define MCP_RXB0SIDH           0x61U
#define MCP_RXB1CTRL           0x70U
#define MCP_RXB1SIDH           0x71U

#define MCP_MODE_MASK          0xE0U
#define MCP_MODE_NORMAL        0x00U
#define MCP_MODE_CONFIG        0x80U
#define MCP_TXREQ              0x08U
#define MCP_RX0IF              0x01U
#define MCP_RX1IF              0x02U

static void MCP_Select(void)
{
    HAL_GPIO_WritePin(MCP2515_CS_PORT, MCP2515_CS_PIN, GPIO_PIN_RESET);
}

static void MCP_Unselect(void)
{
    HAL_GPIO_WritePin(MCP2515_CS_PORT, MCP2515_CS_PIN, GPIO_PIN_SET);
}

static HAL_StatusTypeDef MCP_Command(uint8_t command)
{
    HAL_StatusTypeDef status;

    MCP_Select();
    status = HAL_SPI_Transmit(&hspi1, &command, 1U, 100U);
    MCP_Unselect();
    return status;
}

static HAL_StatusTypeDef MCP_Read(uint8_t address, uint8_t *data,
                                  uint8_t length)
{
    uint8_t command[2] = {MCP_CMD_READ, address};
    HAL_StatusTypeDef status;

    MCP_Select();
    status = HAL_SPI_Transmit(&hspi1, command, sizeof(command), 100U);
    if (status == HAL_OK)
    {
        status = HAL_SPI_Receive(&hspi1, data, length, 100U);
    }
    MCP_Unselect();
    return status;
}

static HAL_StatusTypeDef MCP_Write(uint8_t address, const uint8_t *data,
                                   uint8_t length)
{
    uint8_t command[2] = {MCP_CMD_WRITE, address};
    HAL_StatusTypeDef status;

    MCP_Select();
    status = HAL_SPI_Transmit(&hspi1, command, sizeof(command), 100U);
    if (status == HAL_OK)
    {
        status = HAL_SPI_Transmit(&hspi1, (uint8_t *)data, length, 100U);
    }
    MCP_Unselect();
    return status;
}

static HAL_StatusTypeDef MCP_BitModify(uint8_t address, uint8_t mask,
                                       uint8_t value)
{
    uint8_t command[4] = {MCP_CMD_BIT_MODIFY, address, mask, value};
    HAL_StatusTypeDef status;

    MCP_Select();
    status = HAL_SPI_Transmit(&hspi1, command, sizeof(command), 100U);
    MCP_Unselect();
    return status;
}

static HAL_StatusTypeDef MCP_SetMode(uint8_t mode)
{
    uint8_t canstat;
    uint32_t start = HAL_GetTick();

    if (MCP_BitModify(MCP_CANCTRL, MCP_MODE_MASK, mode) != HAL_OK)
    {
        return HAL_ERROR;
    }

    do
    {
        if (MCP_Read(MCP_CANSTAT, &canstat, 1U) != HAL_OK)
        {
            return HAL_ERROR;
        }

        if ((canstat & MCP_MODE_MASK) == mode)
        {
            return HAL_OK;
        }
    } while ((HAL_GetTick() - start) < 100U);

    return HAL_TIMEOUT;
}

HAL_StatusTypeDef MCP2515_Init(void)
{
    uint8_t value;

    MCP_Unselect();
    if (MCP_Command(MCP_CMD_RESET) != HAL_OK)
    {
        return HAL_ERROR;
    }
    HAL_Delay(10U);

    if (MCP_SetMode(MCP_MODE_CONFIG) != HAL_OK)
    {
        return HAL_ERROR;
    }

    /*
     * 8 MHz MCP2515 crystal, 500 kbit/s:
     * 8 time quanta, sample point 62.5%.
     * Change these three values if your crystal or CAN bitrate differs.
     */
    value = 0x00U;
    if (MCP_Write(MCP_CNF1, &value, 1U) != HAL_OK) return HAL_ERROR;
    value = 0x90U;
    if (MCP_Write(MCP_CNF2, &value, 1U) != HAL_OK) return HAL_ERROR;
    value = 0x02U;
    if (MCP_Write(MCP_CNF3, &value, 1U) != HAL_OK) return HAL_ERROR;

    /* Accept all valid standard/extended frames in both receive buffers. */
    value = 0x60U;
    if (MCP_Write(MCP_RXB0CTRL, &value, 1U) != HAL_OK) return HAL_ERROR;
    if (MCP_Write(MCP_RXB1CTRL, &value, 1U) != HAL_OK) return HAL_ERROR;

    value = 0x03U;
    if (MCP_Write(MCP_CANINTE, &value, 1U) != HAL_OK) return HAL_ERROR;
    value = 0x00U;
    if (MCP_Write(MCP_CANINTF, &value, 1U) != HAL_OK) return HAL_ERROR;
    if (MCP_Write(MCP_EFLG, &value, 1U) != HAL_OK) return HAL_ERROR;

    return MCP_SetMode(MCP_MODE_NORMAL);
}

HAL_StatusTypeDef MCP2515_Send(const MCP2515_Frame_t *frame)
{
    uint8_t txBuffer[13] = {0};
    uint8_t txctrl;
    uint32_t start;

    if ((frame == NULL) || (frame->dlc > 8U) ||
        ((frame->is_extended == 0U) && (frame->id > 0x7FFU)) ||
        ((frame->is_extended != 0U) && (frame->id > 0x1FFFFFFFUL)))
    {
        return HAL_ERROR;
    }

    /* Wait briefly for TX buffer 0 to become available. */
    start = HAL_GetTick();
    do
    {
        if (MCP_Read(MCP_TXB0CTRL, &txctrl, 1U) != HAL_OK)
        {
            return HAL_ERROR;
        }
        if ((txctrl & MCP_TXREQ) == 0U)
        {
            break;
        }
    } while ((HAL_GetTick() - start) < 20U);

    if ((txctrl & MCP_TXREQ) != 0U)
    {
        return HAL_BUSY;
    }

    if (frame->is_extended != 0U)
    {
        txBuffer[0] = (uint8_t)(frame->id >> 21);
        txBuffer[1] = (uint8_t)(((frame->id >> 13) & 0xE0U) |
                                ((frame->id >> 16) & 0x03U) | 0x08U);
        txBuffer[2] = (uint8_t)(frame->id >> 8);
        txBuffer[3] = (uint8_t)frame->id;
    }
    else
    {
        txBuffer[0] = (uint8_t)(frame->id >> 3);
        txBuffer[1] = (uint8_t)((frame->id & 0x07U) << 5);
    }
    txBuffer[4] = frame->dlc;
    for (uint8_t i = 0U; i < frame->dlc; i++)
    {
        txBuffer[5U + i] = frame->data[i];
    }

    if (MCP_Write(MCP_TXB0SIDH, txBuffer,
                  (uint8_t)(5U + frame->dlc)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return MCP_Command(MCP_CMD_RTS_TX0);
}

HAL_StatusTypeDef MCP2515_Receive(MCP2515_Frame_t *frame)
{
    uint8_t intf;
    uint8_t rxBuffer[13];
    uint8_t address;
    uint8_t flag;

    if (frame == NULL)
    {
        return HAL_ERROR;
    }

    if (MCP_Read(MCP_CANINTF, &intf, 1U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((intf & MCP_RX0IF) != 0U)
    {
        address = MCP_RXB0SIDH;
        flag = MCP_RX0IF;
    }
    else if ((intf & MCP_RX1IF) != 0U)
    {
        address = MCP_RXB1SIDH;
        flag = MCP_RX1IF;
    }
    else
    {
        return HAL_BUSY;
    }

    if (MCP_Read(address, rxBuffer, sizeof(rxBuffer)) != HAL_OK)
    {
        return HAL_ERROR;
    }

    if ((rxBuffer[1] & 0x08U) != 0U)
    {
        frame->is_extended = 1U;
        frame->id = ((uint32_t)rxBuffer[0] << 21) |
                    ((uint32_t)(rxBuffer[1] & 0xE0U) << 13) |
                    ((uint32_t)(rxBuffer[1] & 0x03U) << 16) |
                    ((uint32_t)rxBuffer[2] << 8) |
                     (uint32_t)rxBuffer[3];
    }
    else
    {
        frame->is_extended = 0U;
        frame->id = ((uint32_t)rxBuffer[0] << 3) |
                    ((uint32_t)rxBuffer[1] >> 5);
    }
    frame->dlc = rxBuffer[4] & 0x0FU;
    if (frame->dlc > 8U)
    {
        frame->dlc = 8U;
    }

    for (uint8_t i = 0U; i < frame->dlc; i++)
    {
        frame->data[i] = rxBuffer[5U + i];
    }

    if (MCP_BitModify(MCP_CANINTF, flag, 0U) != HAL_OK)
    {
        return HAL_ERROR;
    }

    return HAL_OK;
}
