#include "taskRS485.h"

RS485t RS485; // Định nghĩa thật tại đúng một chỗ

void Rs485Init(void)
{
    RS485.active = false;
    RS485.Rs485Modbus = new MODBUS(&Serial2, GPIO_RS485_RX, GPIO_RS485_TX);
    uint8_t result = RS485.Rs485Modbus->Begin();
    if (result)
    {
        RS485.active = true;
        printf("RS485 is initialized with RX(%d) TX(%d) \n", GPIO_RS485_RX, GPIO_RS485_TX);
    }
    else
    {
        delete RS485.Rs485Modbus;
        RS485.Rs485Modbus = nullptr;
        RS485.active = false;
    }
}
