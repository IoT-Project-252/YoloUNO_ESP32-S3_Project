#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// Struct for Shared Variables
struct SharedData
{
    float temperature;
    float humidity;
    
    SemaphoreHandle_t mutex;
    SemaphoreHandle_t i2cMutex;
    SemaphoreHandle_t neoPixelMutex;
    
    SemaphoreHandle_t semNormal;
    SemaphoreHandle_t semWarning;
    SemaphoreHandle_t semCritical;
};

// Global instance
extern SharedData* projectSharedData;

// Function to initialize global data
void initGlobalData();

#endif