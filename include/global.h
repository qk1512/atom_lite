#ifndef GLOBAL_H
#define GLOBAL_H

#include "Arduino.h"
#include "time.h"
#include "ESPAsyncWebServer.h"
#include <SPIFFS.h>

#define NUMBER_BUTTONS 1
#define MY_LED 27
#define NUM_PIXELS 1
#define BUTTON_R 26
#define BUTTON_V 32
#define NORMAL_STATE 0

extern bool isButtonCheck;
extern bool isSchedulerCheck;

#endif