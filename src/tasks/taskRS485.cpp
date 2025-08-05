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

    for(uint8_t i = 0; i < num; i++)
}

void soil_7in1_sensor()
{
    byte temperatureRequest[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0x84, 0x04};

}



