#include "global.h"
#include "temp_humi.h"
#include "display_lcd.h"
#include "neo_led.h"
#include "actuators.h"
#include "web_server.h"

void setup() 
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ESP32-S3 IoT Project Boot ===");

    initGlobalData();

    xTaskCreate(temp_humi, "Task Read Temperature & Humidity", 4096, projectSharedData, 1, NULL);
    xTaskCreate(controlNeoLED, "Task Neo LED", 4096, projectSharedData, 1, NULL);
    xTaskCreate(displayLCD, "Display on LCD", 4096, projectSharedData, 1, NULL);
    // Task 3: Temperature & Humidity sensor reading
    xTaskCreate(temp_humi,      "TempHumi",    4096, projectSharedData, 1, NULL);

    // Task 3: LCD display
    xTaskCreate(displayLCD,     "DisplayLCD",  4096, projectSharedData, 1, NULL);

    // Task 4: Actuator control (NeoPixel LED + Fan)
    xTaskCreate(actuatorTask,   "Actuators",   4096, projectSharedData, 1, NULL);

    // Task 4: Web server (AP mode + optional STA)
    xTaskCreate(webServerTask,  "WebServer",   8192, projectSharedData, 1, NULL);

    Serial.println("[Main] All tasks created.");
}

void loop() 
{
    vTaskDelete(NULL);
}