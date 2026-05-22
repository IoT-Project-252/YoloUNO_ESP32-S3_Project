#include "display_lcd.h"

void displayLCD(void *pvParameters)
{
    // Cast the generic pointer back to the SharedData structure pointer
    SharedData* sharedData = (SharedData*) pvParameters;

    // Initialize the LCD object (Address: 33, Columns: 16, Rows: 2)
    LiquidCrystal_I2C lcd(33, 16, 2);
    
    // I2C MUTEX PROTECTION: Acquire the I2C bus lock before initializing the LCD hardware
    // This prevents collisions if another task (e.g., sensor task) is currently using the I2C bus
    if (xSemaphoreTake(sharedData->i2cMutex, portMAX_DELAY) == pdTRUE) 
    {
        lcd.init();
        lcd.backlight();
        lcd.setCursor(0, 0);
        lcd.print("System Ready...");
        
        // Release the I2C mutex immediately after the operation is complete
        xSemaphoreGive(sharedData->i2cMutex);
    }
    
    // Wait for 2 seconds to allow the user to read the startup message
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // I2C MUTEX PROTECTION: Safely clear the LCD screen
    if (xSemaphoreTake(sharedData->i2cMutex, portMAX_DELAY) == pdTRUE) 
    {
        lcd.clear();
        xSemaphoreGive(sharedData->i2cMutex);
    }

    float localTemp = 0.0f;
    float localHum = 0.0f;
    uint8_t state = 1; // 0: Normal, 1: Warning, 2: Critical

    while (1) 
    {
        if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(50)) == pdTRUE) 
        {
            localTemp = sharedData->temperature;
            localHum = sharedData->humidity;
            xSemaphoreGive(sharedData->mutex);
        }

        if (xSemaphoreTake(sharedData->semTempNormal, 0) == pdTRUE) {
            state = 0;
        } else if (xSemaphoreTake(sharedData->semTempWarning, 0) == pdTRUE) {
            state = 1;
        } else if (xSemaphoreTake(sharedData->semTempCritical, 0) == pdTRUE) {
            state = 2;
        }

        const char* stateLine = "STATE: NORMAL";
        if (state == 1) {
            stateLine = "STATE: WARNING";
        } else if (state == 2) {
            stateLine = "STATE: CRITICAL";
        }

        if (xSemaphoreTake(sharedData->i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print(stateLine);
            lcd.setCursor(0, 1);
            lcd.print("T:"); lcd.print(localTemp, 1); lcd.print("C H:"); lcd.print(localHum, 0); lcd.print("%");
            lcd.backlight();
            xSemaphoreGive(sharedData->i2cMutex);
        }

        if (state == 2) {
            vTaskDelay(pdMS_TO_TICKS(400));
            if (xSemaphoreTake(sharedData->i2cMutex, portMAX_DELAY) == pdTRUE) 
            {
                lcd.noBacklight();
                xSemaphoreGive(sharedData->i2cMutex);
            }
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}