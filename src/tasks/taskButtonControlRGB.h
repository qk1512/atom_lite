#ifndef BUTTON_TASK_H
#define BUTTON_TASK_H

#include "global.h"
#include "Adafruit_NeoPixel.h"

extern bool statusLed[NUMBER_BUTTONS];
extern TaskHandle_t TaskButton;

extern SemaphoreHandle_t ledStatusMutex;

// void badButtonTask(void *pvParameters);
void initSetUpTaskButton();
void changeStatusLedRGB(int i);
void setLedStatus(int i, bool status);
bool getLedStatus(int i);

#endif
