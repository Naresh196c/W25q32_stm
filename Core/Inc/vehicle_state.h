#ifndef VEHICLE_STATE_H
#define VEHICLE_STATE_H

#include <stdint.h>

typedef struct
{
    uint16_t Voltage_dV;
    int16_t Current_dA;
    uint8_t SOC;
    uint8_t SOH;
    uint16_t Cycles;
    uint16_t Capacity;
    uint8_t Switch_Status;
    int8_t Ambient_Temp_C;
    int8_t MOSFET_Temp_C;
    uint8_t Alive;
    uint16_t RxTimeoutMs;

    uint8_t BmsUniqueIdRaw[8];
    uint16_t BatteryVoltage_raw;
    uint16_t BatteryCurrent_raw;
    int8_t BatteryTemp_C;
    uint8_t LoadStatus;
    uint8_t Status;
    uint16_t BatteryCapacity_raw;
    uint8_t Charge;
    int8_t CellThermistor_C[10];
    char ProtocolVersion[8];
    char BatterySerialNumber[8];
    uint8_t AlarmBits_1;
    uint8_t AlarmBits_2;
    uint8_t ProtBits_1;
    uint8_t ProtBits_2;
    uint8_t FaultBits_1;
    uint16_t CellVoltage_raw_mV[16];
} BatteryState_t;

extern BatteryState_t VS_Battery1;
extern BatteryState_t VS_Battery2;

#endif
