#include "taskConfigTime.h"

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

TaskHandle_t TaskConfigTime = NULL;

void taskConfigTime(void *pvParameters)
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Not get time!");
        return;
    }

    Serial.println("Get time successfully:");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

    while (true)
    {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo))
        {
            // Serial.print("Current Time: ");
            // Serial.println(&timeinfo, "%H:%M:%S");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void initSetUpConfigTime()
{
    xTaskCreate(taskConfigTime, "ConfigTimeTask", 2048, NULL, 1, &TaskConfigTime);
}