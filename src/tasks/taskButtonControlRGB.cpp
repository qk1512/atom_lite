#include "taskButtonControlRGB.h"

Adafruit_NeoPixel led_rgb(NUM_PIXELS, MY_LED, NEO_GRB + NEO_KHZ800);

bool statusLed[NUMBER_BUTTONS] = {false};
int BUTTON[NUMBER_BUTTONS];
int TIMEOUTFORPRESS[NUMBER_BUTTONS];

TaskHandle_t TaskButton = NULL;
SemaphoreHandle_t ledStatusMutex = NULL; // Thêm mutex

int keyReg0[NUMBER_BUTTONS] = {NORMAL_STATE};
int keyReg1[NUMBER_BUTTONS] = {NORMAL_STATE};
int keyReg2[NUMBER_BUTTONS] = {NORMAL_STATE};
int keyReg3[NUMBER_BUTTONS] = {NORMAL_STATE};
int timeOutKeyForPress[NUMBER_BUTTONS] = {0};

void turnOnLedRGB()
{
    led_rgb.begin();
    led_rgb.setPixelColor(0, led_rgb.Color(255, 0, 0));
    led_rgb.show();
}

void turnOffLedRGB()
{
    led_rgb.begin();
    led_rgb.setPixelColor(0, led_rgb.Color(0, 0, 0));
    led_rgb.show();
}

void changeStatusLedRGB(int i)
{
    if (xSemaphoreTake(ledStatusMutex, pdMS_TO_TICKS(100) == pdTRUE))
    {
        if (statusLed[i] == true)
        {
            turnOnLedRGB();
            Serial.println("Turn On Led");
        }
        else
        {
            turnOffLedRGB();
            Serial.println("Turn Off Led");
        }
        xSemaphoreGive(ledStatusMutex);
    }
}

// Thread-safe getter/setter
bool getLedStatus(int i)
{
    bool status = false;
    if (xSemaphoreTake(ledStatusMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        status = statusLed[i];
        xSemaphoreGive(ledStatusMutex);
    }
    return status;
}

void setLedStatus(int i, bool status)
{
    if (xSemaphoreTake(ledStatusMutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        statusLed[i] = status;
        xSemaphoreGive(ledStatusMutex);
    }
}

void goodButtonTask(void *pvParameters)
{
    while (true)
    {
        for (int i = 0; i < NUMBER_BUTTONS; i++)
        {
            keyReg0[i] = keyReg1[i];
            keyReg1[i] = keyReg2[i];
            keyReg2[i] = digitalRead(BUTTON[i]);

            if ((keyReg0[i] == keyReg1[i]) & (keyReg1[i] == keyReg2[i]))
            {
                if (keyReg2[i] != keyReg3[i])
                {
                    keyReg3[i] = keyReg2[i];

                    if (keyReg2[i] == LOW)
                    {
                        timeOutKeyForPress[i] = TIMEOUTFORPRESS[i];
                        bool currentStatus = getLedStatus(i);
                        setLedStatus(i, !currentStatus);
                        changeStatusLedRGB(i);
                    }
                }
                else
                {
                    if (keyReg2[i] == LOW)
                    {
                        timeOutKeyForPress[i]--;
                        if (timeOutKeyForPress[i] == 0)
                        {
                            timeOutKeyForPress[i] = TIMEOUTFORPRESS[i];
                            bool currentStatus = getLedStatus(i);
                            setLedStatus(i, !currentStatus);
                            changeStatusLedRGB(i);
                        }
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void initSetUpTaskButton()
{
    // Tạo mutex trước khi tạo task
    ledStatusMutex = xSemaphoreCreateMutex();
    if (ledStatusMutex == NULL)
    {
        Serial.println("Failed to create mutex!");
        return;
    }

    BUTTON[0] = 26;
    for (int i = 0; i < NUMBER_BUTTONS; i++)
    {
        pinMode(BUTTON[i], INPUT_PULLUP); // Thêm pullup
        TIMEOUTFORPRESS[i] = 60;
    }

    led_rgb.begin();

    // Tăng stack size
    xTaskCreate(goodButtonTask, "ButtonTask", 4096, NULL, 1, &TaskButton);
}