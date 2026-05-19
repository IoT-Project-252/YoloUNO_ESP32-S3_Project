#include "global.h"
#include "temp_humi.h"
#include "display_lcd.h"
#include "actuators.h"
#include "web_server.h"
#include "pir_mqtt.h"

void setup() 
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== ESP32-S3 IoT Project Boot ===");

    initGlobalData();

    // Task 3: Temperature & Humidity sensor reading
    // xTaskCreate(temp_humi,      "TempHumi",    4096, projectSharedData, 1, NULL);

    // Task 3: LCD display
    // xTaskCreate(displayLCD,     "DisplayLCD",  4096, projectSharedData, 1, NULL);

    // Task 4: Actuator control (NeoPixel LED + Fan)
    // xTaskCreate(actuatorTask,   "Actuators",   4096, projectSharedData, 1, NULL);

    // Task 4: Web server (AP mode + optional STA)
    xTaskCreate(webServerTask,  "WebServer",   8192, projectSharedData, 1, NULL);

    // MQTT Extension: PIR sensor detection + MQTT publish
    // This task waits for WiFi STA before connecting to MQTT broker.
    xTaskCreate(pirMqttTask,    "PirMQTT",     6144, projectSharedData, 1, NULL);

    Serial.println("[Main] All tasks created.");
}

void loop() 
{
    vTaskDelete(NULL);
}
