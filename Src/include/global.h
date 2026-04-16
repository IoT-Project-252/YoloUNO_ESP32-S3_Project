#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// Struct for Shared Variables
struct SensorData 
{
    float temperature;
    float humidity;
    
    SemaphoreHandle_t mutex;
    
    SemaphoreHandle_t semNormal;
    SemaphoreHandle_t semWarning;
    SemaphoreHandle_t semCritical;
};

#endif