#include "global.h"
#include "temp_humi.h"


void setup() 
{
    Serial.begin(115200);
    
    initGlobalData();

    // xTaskCreate()
}

void loop() 
{
    vTaskDelete(NULL);
}