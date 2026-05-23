#include "global.h"
#include "temp_humi.h"
#include "display_lcd.h"
#include "neo_led.h"
#include "actuators.h"
#include "web_server.h"
#include "pir_mqtt.h"
#include "task_wifi.h"
#include "task_coreiot.h"
#include "led_blinky.h"
#include "tinyML.h"

void setup() 
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ESP32-S3 IoT Project Boot ===");

    initGlobalData();

    xTaskCreate(temp_humi, "Task Read Temperature & Humidity", 4096, projectSharedData, 1, NULL);
    xTaskCreate(controlNeoLED, "Task Neo LED", 4096, projectSharedData, 1, NULL);
    xTaskCreate(displayLCD, "Display on LCD", 4096, projectSharedData, 1, NULL);
    xTaskCreate(led_blinky, "Task LED Blinky", 4096, projectSharedData, 1, NULL);
    xTaskCreate(tinyMLTask, "TinyML", 6144, projectSharedData, 1, NULL);
    xTaskCreate(actuatorTask, "Actuators", 4096, projectSharedData, 1, NULL);
    xTaskCreate(webServerTask, "WebServer", 8192, projectSharedData, 1, NULL);
    xTaskCreate(WiFi_connect, "WiFi Connect", 4096, projectSharedData, 1, NULL);
    xTaskCreate(CoreIoT_Task, "CoreIOT Task", 8192, projectSharedData, 2, NULL);
    // MQTT Extension: PIR sensor detection + MQTT publish
    xTaskCreate(pirMqttTask, "PirMQTT", 6144, projectSharedData, 1, NULL);

    Serial.println("[Main] All tasks created.");
}

void loop() 
{
    vTaskDelete(NULL);
}