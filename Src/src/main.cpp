#include "global.h"
#include "temp_humi.h"
#include "display_lcd.h"

void setup() 
{
    Serial.begin(115200);
    
    initGlobalData();

    xTaskCreate(temp_humi, "Task Read Temperature & Humidity", 4096, projectSharedData, 1, NULL);
    xTaskCreate(displayLCD, "Display on LCD", 4096, projectSharedData, 1, NULL);
}

void loop() 
{
    vTaskDelete(NULL);
}