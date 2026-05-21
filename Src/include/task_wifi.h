#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include "global.h"
#include <WiFi.h>

// Function prototype for WiFi reconnection logic
bool WiFi_reconnect(SharedData* sharedData);

// Main RTOS task for WiFi connection
void WiFi_connect(void *pvParameters);

#endif