#include "global.h"

// Memory allocation for Shared Datas
SharedData* projectSharedData = nullptr;

// Function to initialize global data
void initGlobalData()
{
    projectSharedData = new SharedData();
    
    projectSharedData->temperature = 0.0f;
    projectSharedData->humidity = 0.0f;
    
    // Initialize Mutex & Semaphores
    projectSharedData->mutex = xSemaphoreCreateMutex();
    projectSharedData->semNormal = xSemaphoreCreateBinary();
    projectSharedData->semWarning = xSemaphoreCreateBinary();
    projectSharedData->semCritical = xSemaphoreCreateBinary();
}