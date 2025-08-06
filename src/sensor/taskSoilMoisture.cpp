#include "taskSoilMoisture.h"
/*
Inquiry Frame
--------------------------------------------------------------------------------------
Address Code |Function Code |Reg start address |Reg length |CRC_L |CRC_H  |
-------------|--------------|------------------|-----------|------|-------|-----------
1byte        |1byte         |2bytes            |2bytest    |1byte |1byte  |
--------------------------------------------------------------------------------------

Answer Frame
-------------------------------------------------------------------------------------------------------
Address Code |Function Code |Enumber of bytes |Data area |Second Data Area |Nth Data Area |Check code
-------------|--------------|-----------------|----------|-----------------|--------------|------------
1byte        |1byte         |1byte            |2bytes    |2bytes           |2bytes        |2bytes
-------------------------------------------------------------------------------------------------------

--------------------------------------------------------
PH Values (unit 0.01pH)          |0006H                |
---------------------------------|---------------------|
Soil Moisture(unit 0.1%RH)       |0012H                |
---------------------------------|---------------------|
Soil Temperature(unit 0.1°C)     |0013H                |
---------------------------------|---------------------|
Soil Conductivity(unit 1us/cm)   |0015H                |
---------------------------------|---------------------|
Soil Nitrogen (unit mg/kg)       |001EH                |
---------------------------------|---------------------|
Soil Phosphorus (unit mg/kg)     |001FH                |
---------------------------------|---------------------|
Soil Potassium (unit mg/kg)      |0020H                |
--------------------------------------------------------

This transmitter only uses function code 0x03 (read register data)
*/

//#ifdef USE_RS485
//#ifdef USE_SOILMOISTURE

SoilMoisture SM_sensor;

bool SMisConnected()
{
    if(!RS485.active) return false;
    RS485.Rs485Modbus -> sendModbusRequest(SM_ADDRESS_ID, SM_FUNCTION_CODE, (0x01 << 8) | 0x00, 0x01);

    delay(200);

    RS485.Rs485Modbus -> receiveReady();

    uint8_t buffer[8];
    uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,8);
    if(error)
    {
        Serial.printf("Soil Moisture has error %d\n",error);
        return false;
    }
    else
    {
        uint16_t check_SM = (buffer[3] << 8) | buffer[4];
        if(check_SM == SM_ADDRESS_ID) return true;
    }
    return false;
}

void SMInit(void)
{
    if(!RS485.active)
    {
        return;
    }
    SM_sensor.valid = SMisConnected();

    //if(!SM_sensor.valid)
    Serial.printf(SM_sensor.valid ? "Soil Moisture is connected\n" : "Soil Moisture is not detected\n");

}

void SMReadData(void)
{
    if(SM_sensor.valid == false) return;

    static const struct 
    {
        uint16_t regAddr;
        uint8_t regCount;
    } modbusRequest[] = 
    {
        {0x0006, 1}, // PH Value
        {0x0012, 2}, // Soil Moisture & Temperature
        {0x0015, 1}, // Soil Conductivity
        {0x001E, 3}  // Nitrogen, Phosphorus, Potassium
    };
     
    static uint8_t requestIndex = 0;

    RS485.Rs485Modbus -> sendModbusRequest(SM_ADDRESS_ID, SM_FUNCTION_CODE, modbusRequest[requestIndex].regAddr, modbusRequest[requestIndex].regCount);
    
    delay(200);

    if(RS485.Rs485Modbus -> receiveReady())
    {
        uint8_t buffer[10];
        uint8_t error = RS485.Rs485Modbus -> ReceiveBuffer(buffer,sizeof(buffer));

        if(error)
        {
            Serial.printf("Modbus Error: %u", error);
        }
        else
        {
            switch (requestIndex)
            {
            case 0: // PH Value
                SM_sensor.PH_values = ((buffer[3] << 8) | buffer[4]) / 100.0;
                break;
            case 1: // Soil Moisture & Temperature
                SM_sensor.soil_moisture = ((buffer[3] << 8) | buffer[4]) / 10.0;
                SM_sensor.temperature = ((buffer[5] << 8) | buffer[6]) / 10.0;
                break;
            case 2: // Soil Conductivity
                SM_sensor.soil_conductivity = (buffer[3] << 8) | buffer[4];
                break;
            case 3: // Nitrogen, Phosphorus, Potassium
                SM_sensor.soil_nitrogen = (buffer[3] << 8) | buffer[4];
                SM_sensor.soil_phosphorus = (buffer[5] << 8) | buffer[6];
                SM_sensor.soil_potassium = (buffer[7] << 8) | buffer[8];
                break;
            }
        }
        requestIndex = (requestIndex + 1) % (sizeof(modbusRequest) / sizeof(modbusRequest[0]));
    }
}

//pH_EC_soilTemp_soilHumid_N_P_K

void SMPrintData(void)
{
    printf("PH Values: %f \n", SM_sensor.PH_values);
    printf("Soil Moisture: %f \n", SM_sensor.soil_moisture);
    printf("Temperature: %f \n", SM_sensor.temperature);
    printf("Soil Conductivity: %d \n", SM_sensor.soil_conductivity);
    printf("Soil Nitrogen: %d \n", SM_sensor.soil_nitrogen);
    printf("Soil Phosphorus: %d \n", SM_sensor.soil_phosphorus);
    printf("Soil Potassium: %d \n", SM_sensor.soil_potassium);
}