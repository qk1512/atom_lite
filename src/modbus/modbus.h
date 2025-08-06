#ifndef MODBUS_H
#define MODBUS_H

#include <Arduino.h>
#include <HardwareSerial.h>

#define MODBUS_BAUDRATE 9600


class MODBUS
{
    public:
    MODBUS(HardwareSerial* serial,int receive_pin, int transmit_pin, int tx_enable_pin = 2);

    bool Begin(long speed = MODBUS_BAUDRATE , uint32_t config = SERIAL_8N1);
    uint8_t sendModbusRequest(uint8_t device_address, uint8_t function_code, uint16_t start_address, uint16_t count, uint16_t *writeData = NULL);
    uint8_t ReceiveBuffer(uint8_t *buffer, uint8_t register_count, uint16_t byte_count = 0);
    uint16_t CalculateCRC(uint8_t *frame, uint8_t num);
    bool receiveReady();
    
    private : 

    int rx_pin;
    int tx_pin;

    void setTransmitEnablePin(int8_t txEnablePin);
    HardwareSerial *ModbusSerial;
    int mb_tx_enable_pin;
    uint8_t mb_address;
    uint8_t mb_len;

    int8_t m_txEnablePin = -1;
    bool m_rxValid = false;
    bool m_rxEnabled = false;
    bool m_txValid = false;
    bool m_txEnableValid = false;
};

#endif