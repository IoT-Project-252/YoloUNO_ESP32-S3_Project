#include "global.h"

// Memory allocation for Shared Datas
SharedData* projectSharedData = nullptr;

// Function to initialize global data
void initGlobalData()
{
    projectSharedData = new SharedData();

    // Sensor defaults
    projectSharedData->temperature = 0.0f;
    projectSharedData->humidity = 0.0f;

    // LED RGB defaults (off, black)
    projectSharedData->ledOn      = false;
    projectSharedData->ledR       = 0;
    projectSharedData->ledG       = 0;
    projectSharedData->ledB       = 0;
    projectSharedData->ledChanged = false;

    // Fan defaults (off, 0%)
    projectSharedData->fanOn      = false;
    projectSharedData->fanSpeed   = 0;
    projectSharedData->fanChanged = false;

    // WiFi STA defaults
    projectSharedData->staConnected = false;
    memset(projectSharedData->staSSID, 0, sizeof(projectSharedData->staSSID));

    // Initialize Mutex & Semaphores
    projectSharedData->mutex         = xSemaphoreCreateMutex();
    projectSharedData->i2cMutex      = xSemaphoreCreateMutex();
    projectSharedData->actuatorMutex = xSemaphoreCreateMutex();

    projectSharedData->semNormal   = xSemaphoreCreateBinary();
    projectSharedData->semWarning  = xSemaphoreCreateBinary();
    projectSharedData->semCritical = xSemaphoreCreateBinary();
}