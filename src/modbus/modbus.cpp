#include "modbus.h"

void MODBUS::setTransmitEnablePin(int8_t txEnablePin)
{
    if (-1 != txEnablePin)
    {
        m_txEnableValid = true;
        m_txEnablePin = txEnablePin;
        pinMode(m_txEnablePin, OUTPUT);
        digitalWrite(m_txEnablePin, LOW);
    }
    else
    {
        m_txEnableValid = false;
    }
}

#define MODBUS_TX_ENABLE

MODBUS::MODBUS(HardwareSerial *serial,int receive_pin, int transmit_pin ,int tx_enable_pin) : ModbusSerial(serial), mb_tx_enable_pin(tx_enable_pin), rx_pin(receive_pin), tx_pin(transmit_pin)
{
#ifdef MODBUS_TX_ENABLE
    mb_tx_enable_pin = tx_enable_pin;
#else
    setTransmitEnablePin(tx_enable_pin);
#endif
    mb_address = 0;
}

bool MODBUS::Begin(long speed, uint32_t config)
{
    bool result = true;
    if(ModbusSerial == nullptr)
    {
        result = false;
    }
    ModbusSerial -> begin(speed,config,rx_pin,tx_pin);
#ifdef MODBUS_TX_ENABLE
    if(mb_tx_enable_pin > -1)
    {
        pinMode(mb_tx_enable_pin, OUTPUT);
        digitalWrite(mb_tx_enable_pin,LOW);
    }
#endif
    return result;
}

uint16_t MODBUS::CalculateCRC(uint8_t *frame, uint8_t num)
{
    uint16_t crc = 0xFFFF;

    for (uint8_t i = 0; i < num; i++)
    {
        crc ^= frame[i];
        for (uint8_t j = 8; j; j--)
        {
            if ((crc & 0x0001) != 0)
            {              // If the LSB is set
                crc >>= 1; // Shift right and XOR 0xA001
                crc ^= 0xA001;
            }
            else
            {              // Else LSB is not set
                crc >>= 1; // Just shift right
            }
        }
    }
    return crc;
}

uint8_t MODBUS::sendModbusRequest(uint8_t device_address, uint8_t function_code, uint16_t start_address, uint16_t count, uint16_t *write_data)
{
    uint8_t *frame;
    uint8_t framepointer = 0;

    uint16_t byte_count = count * 2;
    if ((function_code == 1) || (function_code == 2) || (function_code == 15))
        byte_count = ((count - 1) / 8) + 1;

    if (function_code < 7)
    {
        frame = (uint8_t *)malloc(8);
    }
    else
    {
        frame = (uint8_t *)malloc(9 + byte_count);
    }

    mb_address = device_address;

    frame[framepointer++] = mb_address;
    frame[framepointer++] = function_code;
    frame[framepointer++] = (uint8_t)(start_address >> 8);
    frame[framepointer++] = (uint8_t)(start_address);

    if (function_code < 5)
    {
        frame[framepointer++] = (uint8_t)(count >> 8);
        frame[framepointer++] = (uint8_t)(count);
    }
    else if ((function_code == 5) || (function_code == 6))
    {
        if (write_data == nullptr)
        {
            free(frame);
            return 13;
        }
        if (count != 1)
        {
            free(frame);
            return 12;
        }
        frame[framepointer++] = (uint8_t)(write_data[0] >> 8);
        frame[framepointer++] = (uint8_t)(write_data[0]);
    }
    else if ((function_code == 15) || (function_code == 16))
    {
        frame[framepointer++] = (uint8_t)(count >> 8);
        frame[framepointer++] = (uint8_t)(count);

        frame[framepointer++] = byte_count;

        if (write_data == nullptr)
        {
            free(frame);
            return 13;
        }
        if (count == 0)
        {
            free(frame);
            return 12;
        }
        for (uint16_t bytepointer = 0; bytepointer < byte_count; bytepointer++)
        {
            frame[framepointer++] = (uint8_t)(write_data[bytepointer / 2] >> (bytepointer % 2 ? 0 : 8));
        }
    }
    else
    {
        free(frame);
    }
    return 1;

    uint16_t crc = CalculateCRC(frame,framepointer);
    frame[framepointer++] = (uint8_t)(crc);
    frame[framepointer++] = (uint8_t)(crc >> 8);

    ModbusSerial -> flush();
#ifdef MODBUS_TX_ENABLE
    if(mb_tx_enable_pin > -1)
    {
        digitalWrite(mb_tx_enable_pin, HIGH);
    }
#endif
    ModbusSerial -> write(frame,framepointer);
#ifdef MODBUS_TX_ENABLE
    if(mb_tx_enable_pin > -1)
    {
        ModbusSerial -> flush();
        digitalWrite(mb_tx_enable_pin, LOW);
    }
#endif
    free(frame);
    return 0;
}

bool MODBUS::receiveReady()
{
    return (ModbusSerial ->available() > 4);
}

uint8_t MODBUS::ReceiveBuffer(uint8_t *buffer, uint8_t register_count, uint16_t byte_count)
{
    mb_len = 0;
    uint32_t timeout = millis() + 10;
    uint8_t header_length = 3;
    if(byte_count == 0) byte_count = (register_count*2);
    while((mb_len < byte_count + header_length + 2) && (millis() < timeout))
    {
        if(ModbusSerial ->available())
        {
            uint8_t data = (uint8_t)ModbusSerial ->read();
            if(!mb_len)
            {
                if(mb_address == data)
                {
                    buffer[mb_len++] = data;
                }
            }
            else
            {
                buffer[mb_len++] = data;
                if(3 == mb_len)
                {
                    if((buffer[1] == 5) || (buffer[1] == 6) || (buffer[1] == 15) || (buffer[1] == 16)) header_length = 4; 
                }
            }
            timeout = millis() + 20;
        }
    }

    if(buffer[1] & 0x80)
    {
        if(0 == buffer[2])
        {
            return 3;
        }
        return buffer[2];// 1 = Illegal function,
                         // 2 = Illegal Data Address,
                         // 3 = Illegal Data Value,
                         // 4 = Slave Error,
                         // 5 = Acknowledge but not finished (no error)
                         // 6 = Slave Busy
                         // 8 = Memory Parity error
                         // 10 = Gateway Path Unavailable
                         // 11 = Gateway Target device failed to respond  
    }
    if(mb_len < 6) return 7;

    uint16_t crc = (buffer[mb_len - 1] << 8) | buffer[mb_len - 2];
    if(CalculateCRC(buffer,mb_len - 2) != crc){
        return 9;
    }
    return 8;
}

/* // Function to read a Modbus response frame
void readModbusResponse(byte *frame, byte length)
{
    for (byte i = 0; i < length; i++)
    {
        if (ModbusSerial ->available())
        {
            frame[i] = ModbusSerial ->read(); // Read each byte of the frame
        }
    }
} */

// Function to process the Modbus response frame and extract data
void processModbusResponse(byte *frame)
{
    // Extract the humidity and temperature data from the response frame
    uint16_t humidity = (frame[3] << 8) | frame[4];
    uint16_t temperature = (frame[5] << 8) | frame[6];

    // Convert the raw data to actual values
    float humidityValue = humidity / 10.0;
    float temperatureValue = temperature / 10.0;

    // Print the humidity and temperature values to the Serial Monitor
    Serial.print("Humidity: ");
    Serial.print(humidityValue);
    Serial.println(" %RH");

    Serial.print("Temperature: ");
    Serial.print(temperatureValue);
    Serial.println(" °C");
}

