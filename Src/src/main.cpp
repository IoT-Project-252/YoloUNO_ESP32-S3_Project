#include "global.h"
#include "temp_humi.h"
#include "display_lcd.h"
#include "neo_led.h"
#include "actuators.h"
#include "web_server.h"
#include "pir_mqtt.h"
#include "task_wifi.h"
#include "task_coreiot.h"

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
    // xTaskCreate(temp_humi,      "TempHumi",    4096, projectSharedData, 1, NULL);

    // Task 3: LCD display
    // xTaskCreate(displayLCD,     "DisplayLCD",  4096, projectSharedData, 1, NULL);

    // Task 4: Actuator control (NeoPixel LED + Fan)
    // xTaskCreate(actuatorTask,   "Actuators",   4096, projectSharedData, 1, NULL);

    // Task 4: Web server (AP mode + optional STA)
    xTaskCreate(webServerTask,  "WebServer",   8192, projectSharedData, 1, NULL);

    // Task 6: WiFi monitoring task
    xTaskCreate(WiFi_connect, "WiFi Connect", 4096, projectSharedData, 1, NULL);

    // Task 6: Data Publishing to CoreIOT Cloud Server
    xTaskCreate(CoreIoT_Task, "CoreIOT Task", 8192, projectSharedData, 2, NULL);

    // MQTT Extension: PIR sensor detection + MQTT publish
    // This task waits for WiFi STA before connecting to MQTT broker.
    xTaskCreate(pirMqttTask,    "PirMQTT",     6144, projectSharedData, 1, NULL);

    Serial.println("[Main] All tasks created.");
}

void loop() 
{
    vTaskDelete(NULL);
}
