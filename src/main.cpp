// main.cpp - Cải thiện
#include "global.h"
//#include "./tasks/taskButtonControlRGB.h"
#include "./tasks/taskInitWifi.h"
#include "./tasks/taskInitThingsBorad.h"
#include "./tasks/taskConfigTime.h"
//#include "./tasks/taskSetUpWebServer.h"
#include "./tasks/taskSchedule.h"
#include "./tasks/taskRS485.h"
#include "./sensor/taskSoilMoisture.h"

void setup()
{
  Serial.begin(115200);

  Rs485Init();

  delay(1000); // Cho serial port ổn định
  Serial.println("M5Atom Lite Start program");

  // Khởi tạo theo thứ tự logic
  //initSetUpWifiSM();

  // Đợi WiFi connect
  //delay(1000);

  //initSetUpThingBoard();

  //delay(1000);

  //initSetUpConfigTime();
  //initSetUpTaskButton();
  //initSetUpScheduler();

  SMInit();


  // Kiểm tra đúng task handle
  if (TaskButton != NULL && TaskScheduler != NULL)
  {
    //initSetUpWebServer();
    Serial.println("All tasks initialized successfully");
  }
  else
  {
    Serial.println("Failed to initialize some tasks");
  }

  // In thông tin memory
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
}

void loop()
{
  vTaskDelay(pdMS_TO_TICKS(1000));
  if(SM_sensor.valid)
  {
    SMReadData();
    SMPrintData();
  }
}