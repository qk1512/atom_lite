#ifndef SOIL_MOISTURE
#define SOIL_MOISTURE

#include "./tasks/taskRS485.h"

#define SM_ADDRESS_ID 0x01
#define SM_FUNCTION_CODE 0x03
#define SM_ADDRESS_CHECK 0x0100

struct SoilMoisture
{
    float temperature = 0;
    float PH_values = 0;
    float soil_moisture = 0;
    int soil_conductivity = 0;
    int soil_nitrogen = 0;
    int soil_phosphorus = 0;
    int soil_potassium = 0;

    bool valid = false;

};

extern SoilMoisture SM_sensor;

bool SMisConnected();
void SMInit(void);
void SMReadData(void);
void SMPrintData(void);

#endif