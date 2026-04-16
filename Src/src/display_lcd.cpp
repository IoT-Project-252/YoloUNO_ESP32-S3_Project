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

    // Local variables to cache sensor data
    // This minimizes the time the data mutex is held, reducing task blocking time
    float localTemp = 0.0;
    float localHum = 0.0;

    // Main infinite loop for the display task
    while (1) 
    {
        
        // --- NORMAL STATE HANDLING ---
        // Check if the 'Normal' state semaphore is available (0 ticks = non-blocking)
        if (xSemaphoreTake(sharedData->semNormal, 0) == pdTRUE) 
        {
            
            // DATA MUTEX: Securely read the latest temperature and humidity values
            if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(50)) == pdTRUE) 
            {
                localTemp = sharedData->temperature;
                localHum = sharedData->humidity;
                xSemaphoreGive(sharedData->mutex);
            }
            
            // I2C MUTEX PROTECTION: Safely update the LCD with 'Normal' UI
            if (xSemaphoreTake(sharedData->i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
            {
                lcd.clear();
                lcd.setCursor(0, 0); lcd.print("STATE: NORMAL  ");
                lcd.setCursor(0, 1); lcd.print("T:"); lcd.print(localTemp, 1); lcd.print("C H:"); lcd.print(localHum, 0); lcd.print("%");
                xSemaphoreGive(sharedData->i2cMutex);
            }
        }
        
        // --- WARNING STATE HANDLING ---
        else if (xSemaphoreTake(sharedData->semWarning, 0) == pdTRUE) 
        {
            
            // DATA MUTEX: Securely read the latest sensor values
            if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(50)) == pdTRUE) 
            {
                localTemp = sharedData->temperature;
                localHum = sharedData->humidity;
                xSemaphoreGive(sharedData->mutex);
            }
            
            // I2C MUTEX PROTECTION: Safely update the LCD with 'Warning' UI
            if (xSemaphoreTake(sharedData->i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
            {
                lcd.clear();
                lcd.setCursor(0, 0); lcd.print("STATE: WARNING!");
                lcd.setCursor(0, 1); lcd.print("T:"); lcd.print(localTemp, 1); lcd.print("C H:"); lcd.print(localHum, 0); lcd.print("%");
                xSemaphoreGive(sharedData->i2cMutex);
            }
        }
        
        // --- CRITICAL STATE HANDLING ---
        else if (xSemaphoreTake(sharedData->semCritical, 0) == pdTRUE) 
        {
            
            // DATA MUTEX: Securely read the latest sensor values
            if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(50)) == pdTRUE) 
            {
                localTemp = sharedData->temperature;
                localHum = sharedData->humidity;
                xSemaphoreGive(sharedData->mutex);
            }
            
            // I2C MUTEX PROTECTION: Safely update the LCD with 'Critical' UI
            if (xSemaphoreTake(sharedData->i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) 
            {
                lcd.clear();
                lcd.setCursor(0, 0); lcd.print("!!! CRITICAL !!!");
                lcd.setCursor(0, 1); lcd.print("T:"); lcd.print(localTemp, 1); lcd.print("C");
                xSemaphoreGive(sharedData->i2cMutex);
            }
            
            // I2C MUTEX PROTECTION: Handle the blinking backlight effect
            vTaskDelay(pdMS_TO_TICKS(500)); // Keep backlight on for 500ms
            
            if (xSemaphoreTake(sharedData->i2cMutex, portMAX_DELAY) == pdTRUE) 
            {
                lcd.noBacklight();
                xSemaphoreGive(sharedData->i2cMutex);
            }
            
            vTaskDelay(pdMS_TO_TICKS(200)); // Keep backlight off for 200ms
            
            if (xSemaphoreTake(sharedData->i2cMutex, portMAX_DELAY) == pdTRUE) 
            {
                lcd.backlight();
                xSemaphoreGive(sharedData->i2cMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}