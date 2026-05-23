#ifndef TASK_COREIOT_H
#define TASK_COREIOT_H

#include "global.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// --- CoreIOT Server Configuration ---
#define CORE_IOT_SERVER "app.coreiot.io"            // CoreIOT Server
#define CORE_IOT_TOKEN  "unmkldv00sjjmj2ob29d"      // Device Access Token
#define CORE_IOT_PORT   1883

// Task prototype
void CoreIoT_Task(void *pvParameters);

#endif