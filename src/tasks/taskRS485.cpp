#include "taskRS485.h"

//Serial2.begin();
HardwareSerial RS485Serial(2);

void sendRS485Command(byte *command, int commandSize, byte *response, int responseSize);
{
    RS485Serial.write(command, commandSize);
    RS485Serial.flush();
    delay(100);
    if(RS485Serial.available() >= responseSize)
    {
        RS485Serial.readBytes(response, responseSize);
    }
}

uint16_t CalculateCRC(uint8_t *frame, uint8_t num)
{
    uint16_t crc = 0xFFFF;

    for(uint8_t i = 0; i < num; i++){
        crc ^= frame[i];
        for(uint8_t j = 8; j; j--)
        {
            if((crc && 0x0001) != 0)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

uint8_t SendRS485(uint8_t device_address, uint8_t function_code, uint16_t start_address, uint16_t count, uint16_t *write_data)
{
    uint8_t *frame;
    uint8_t framepointer = 0;

#ifdef DEBUGMODE
    //printf_P()
#endif

    uint16_t byte_count = count*2;
    if((function_code == 1) || (function_code == 2) || (function_code == 15)) byte_count = ((count-1)/8) + 1;

    if(function_code < 7)
    {
        frame = (uint8_t *)malloc(8);
    }
    else
    {
        frame = (uint8_t *)malloc(9 + byte_count);
    }

    //mb_address = device_address;

    frame[framepointer++] = device_address;
    frame[framepointer++] = function_code;
    frame[framepointer++] = (uint8_t)(start_address >> 8);
    frame[framepointer++] = (uint8_t)(start_address);
    
    if(function_code < 5)
    {
        frame[framepointer++] = (uint8_t)(count >> 8);
        frame[framepointer++] = (uint8_t)(count);
    }
    else if((function_code == 5) || (function_code == 6))
    {
        if(write_data == nullptr)
        {
            free(frame);
            return 13;
        }
        if(count != 1)
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

        if(write_data == nullptr)
        {
            free(frame);
            return 13;
        }
        if(count == 0)
        {
            free(frame);
            return 12;
        }
        for(uint16_t bytepointer = 0; bytepointer < byte_count; bytepointer++)
        {
            frame[framepointer++] = (uint8_t)(write_data[bytepointer/2] >> (bytepointer % 2 ? 0 : 8));
        }
    }
    else{
        free(frame);
    }
    return 1;
}


void soil_7in1_sensor()
{
    byte temperatureRequest[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x04};

}



