#include "neo_led.h"

void controlNeoLED(void *pvParameters) {
    // Cast the generic task parameter back to the SharedData structure pointer
    SharedData* sharedData = (SharedData*) pvParameters;

    // Initialize the Adafruit_NeoPixel object
    Adafruit_NeoPixel strip(LED_COUNT, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);
    
    // MUTEX PROTECTION: Secure the hardware initialization process
    if (xSemaphoreTake(sharedData->neoPixelMutex, portMAX_DELAY) == pdTRUE) 
    {
        strip.begin();

        strip.clear();                  // Turn off all LEDs initially
        strip.show();                   // Apply changes to the physical hardware
        xSemaphoreGive(sharedData->neoPixelMutex);
    }

    float localHum = 0.0;

    // Main infinite loop for the NeoPixel control task
    while (1) 
    {
        // DATA MUTEX: Securely read the latest humidity value from shared memory
        if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(50)) == pdTRUE) 
        {
            localHum = sharedData->humidity;
            xSemaphoreGive(sharedData->mutex);
        }

        // NEOPIXEL MUTEX PROTECTION: Update LED color patterns safely
        if (xSemaphoreTake(sharedData->neoPixelMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            // Task 2 Logic: Map humidity ranges to 3 distinct colors
            if (localHum < 40.0) 
            {
                // Dry environment: Set color to Yellow
                strip.setPixelColor(0, strip.Color(255, 255, 0));
            } 
            else if (localHum >= 40.0 && localHum <= 70.0) 
            {
                // Optimal environment: Set color to Green
                strip.setPixelColor(0, strip.Color(0, 255, 0));
            } 
            else 
            {
                // High humidity environment: Set color to Red
                strip.setPixelColor(0, strip.Color(255, 0, 0));
            }
            
            // Update the strip with the new color
            strip.show();
            xSemaphoreGive(sharedData->neoPixelMutex);
        }

        // Delay for 1 second before the next iteration loop
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}