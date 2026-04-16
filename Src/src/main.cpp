#include "global.h"



void setup() 
{
    Serial.begin(115200);

    // Memory allocation for Shared Datas
    SensorData* mySharedData = new SensorData();
    
    mySharedData->temperature = 0.0f;
    mySharedData->humidity = 0.0f;

    // Initialize Mutex & Semaphores
    mySharedData->mutex = xSemaphoreCreateMutex();
    mySharedData->semNormal = xSemaphoreCreateBinary();
    mySharedData->semWarning = xSemaphoreCreateBinary();
    mySharedData->semCritical = xSemaphoreCreateBinary();
}

void loop() 
{
    vTaskDelete(NULL);
}