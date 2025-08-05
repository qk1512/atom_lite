#include "taskSetUpWebServer.h"

AsyncWebServer server(80);

TaskHandle_t TaskWebServer = NULL;
bool spiffsInitialized = false;

void changeStatusLedRGBWrapper()
{
    bool status = getLedStatus(0);
    setLedStatus(0, !status);
    changeStatusLedRGB(0);
}

void initSPIFFS()
{
    if (!spiffsInitialized)
    {
        if (SPIFFS.begin(true))
        {
            spiffsInitialized = true;
            Serial.println("SPIFFS mounted successfully");
        }
        else
        {
            Serial.println("SPIFFS mount failed");
        }
    }
}

void taskWebServer(void *pvParameters)
{
    initSPIFFS(); // Chỉ init một lần

    if (!spiffsInitialized)
    {
        Serial.println("Cannot start web server - SPIFFS failed");
        vTaskDelete(NULL);
        return;
    }

    // Setup routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        File file = SPIFFS.open("/index.html", "r");
        if (file) {
            String html = file.readString();
            file.close();
            
            bool ledStatus = getLedStatus(0);
            String stateStr = ledStatus ? "on" : "off";
            String buttonText = ledStatus ? "Turn OFF" : "Turn ON";
            
            html.replace("%STATE%", stateStr);
            html.replace("%BTN_TEXT%", buttonText);
            
            request->send(200, "text/html", html);
        } else {
            request->send(404, "text/plain", "File not found");
        } });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(SPIFFS, "/style.css", "text/css"); });

    server.on("/led/on", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        setLedStatus(0, true);
        changeStatusLedRGB(0);
        request->send(200, "text/plain", "LED turned ON"); });

    server.on("/led/off", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        setLedStatus(0, false);
        changeStatusLedRGB(0);
        request->send(200, "text/plain", "LED turned OFF"); });

    server.on("/set_schedule", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        if (request->hasParam("h") && request->hasParam("m") && request->hasParam("s") && request->hasParam("type"))
        {
            int hour = request->getParam("h")->value().toInt();
            int minute = request->getParam("m")->value().toInt();
            int second = request->getParam("s")->value().toInt();
            String type = request->getParam("type")->value();
            bool active = true;

            // Lấy thời gian hiện tại
            struct tm timeinfo;
            if (getLocalTime(&timeinfo))
            {
                int nowInSeconds = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
                int scheduleInSeconds = hour * 3600 + minute * 60 + second;

                int diffInSeconds = scheduleInSeconds - nowInSeconds;
                if (diffInSeconds < 0)
                {
                    diffInSeconds += 24 * 3600; // cộng thêm 1 ngày nếu thời gian đặt là ngày hôm sau
                }

                Serial.printf("Thời gian còn lại đến lịch là %d giây\n", diffInSeconds);
                SCH_Add_Task(changeStatusLedRGBWrapper, diffInSeconds, 0);
            }
            else
            {
                Serial.println("Không lấy được thời gian hệ thống");
            }

            Serial.printf("Đã đặt lịch bật LED lúc %02d:%02d:%02d, loại: %s\n",
                          hour, minute, second, type.c_str());

            request->send(200, "text/plain", "Lịch đã được đặt");

        } else {
        request->send(400, "text/plain", "Thiếu tham số");
        } });

    server.begin();
    Serial.println("Web server started");

    // Task chỉ cần suspend, không cần loop
    vTaskSuspend(NULL);
}

void initSetUpWebServer()
{
    xTaskCreate(taskWebServer, "WebserverTask", 8192, NULL, 1, &TaskWebServer);
}