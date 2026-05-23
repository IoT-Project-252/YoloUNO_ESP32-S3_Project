#include "neo_led.h"

// Define Color Macros
#define COLOR_YELLOW strip.Color(255, 255, 0)
#define COLOR_GREEN  strip.Color(0, 255, 0)
#define COLOR_RED    strip.Color(255, 0, 0)
#define COLOR_WHITE  strip.Color(255, 255, 255)
#define COLOR_OFF    strip.Color(0, 0, 0)

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
        bool isOverride = false;
        // DATA MUTEX: Securely read the latest humidity value and override flag from shared memory
        if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(50)) == pdTRUE) 
        {
            localHum = sharedData->humidity;
            isOverride = sharedData->forceNeoPixelLED;
            xSemaphoreGive(sharedData->mutex);
        }

        // NEOPIXEL MUTEX PROTECTION: Update LED color patterns safely
        if (xSemaphoreTake(sharedData->neoPixelMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            // Override Mode: Set color to White
            if (isOverride)
            {
                strip.setPixelColor(0, COLOR_WHITE);
            } 
            else
            {
                // Task 2 Logic: Map humidity ranges to 3 distinct colors
                if (localHum < 40.0) 
                {
                    // Dry environment: Set color to Yellow
                    strip.setPixelColor(0, COLOR_YELLOW);
                } 
                else if (localHum >= 40.0 && localHum <= 70.0) 
                {
                    // Optimal environment: Set color to Green
                    strip.setPixelColor(0, COLOR_GREEN);
                } 
                else 
                {
                    // High humidity environment: Set color to Red
                    strip.setPixelColor(0, COLOR_RED);
                }
            }
            
            // Update the strip with the new color
            strip.show();
            xSemaphoreGive(sharedData->neoPixelMutex);
        }

        // Delay for 0.5 second before the next iteration loop
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}