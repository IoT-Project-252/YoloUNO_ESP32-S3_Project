#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// ── Struct for Shared Variables ──────────────────────────────────────────────
struct SharedData
{
    // --- Sensor Data ---
    float temperature;
    float humidity;
    float avg_temp;
    float avg_humi;

    // --- TinyML Classification ---
    uint8_t mlLabel; // 0=Normal, 1=TempAnomaly, 2=HumidityAnomaly

    // --- LED RGB NeoPixel State ---
    bool     ledOn;
    uint8_t  ledR;
    uint8_t  ledG;
    uint8_t  ledB;
    bool     ledChanged;   // flag to signal actuator task

    // --- Fan State ---
    bool     fanOn;
    uint8_t  fanSpeed;     // 0-100 (%)
    bool     fanChanged;   // flag to signal actuator task

    // --- WiFi STA Status ---
    bool     staConnected;
    char     staSSID[33];  // current STA SSID (max 32 chars + null)

    // --- Mutexes ---
    SemaphoreHandle_t mutex;       // protects temperature / humidity
    SemaphoreHandle_t i2cMutex;    // protects I2C bus
    SemaphoreHandle_t actuatorMutex; // protects LED & fan fields
    SemaphoreHandle_t neoPixelMutex;    // Define mutex for NeoPixel LED protection
    SemaphoreHandle_t serialMutex;      // Define mutex for Serial Monitor protection

    // --- Semaphores for LCD state machine ---
    SemaphoreHandle_t semNormal;
    SemaphoreHandle_t semWarning;
    SemaphoreHandle_t semCritical;

    // --- Semaphores for Task 1/3 (separate consumers) ---
    SemaphoreHandle_t semTempNormal;
    SemaphoreHandle_t semTempWarning;
    SemaphoreHandle_t semTempCritical;

    // Handoff signal, sensor -> ML
    SemaphoreHandle_t semDataReady;
};

// Global instance
extern SharedData* projectSharedData;

// Function to initialize global data
void initGlobalData();

#endif