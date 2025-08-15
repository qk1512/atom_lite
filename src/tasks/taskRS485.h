#ifndef INIT_RS485
#define INIT_RS485

#include "Arduino.h"
#include "./modbus/modbus.h"

#define GPIO_RS485_TX 23
#define GPIO_RS485_RX 33

//#define ATOM_DTU_RS485_TX 23
//#define ATOM_DTU_RS485_RX 33

struct RS485t
{
    bool active = false;
    int tx = GPIO_RS485_TX;
    int rx = GPIO_RS485_RX;
    MODBUS *Rs485Modbus = nullptr;
};

extern RS485t RS485;  // Chỉ khai báo biến extern
void Rs485Init(void); // Chỉ khai báo hàm

#endif
