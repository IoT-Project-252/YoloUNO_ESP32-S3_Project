#include "temp_humi.h"

void temp_humi(void *pvParameters)
{
    // Cast the generic task parameter back to the SharedData structure pointer
    SharedData* sharedData = (SharedData*) pvParameters;

    // Instantiate the DHT20 sensor object
    DHT20 dht;

    // Initialize the I2C bus with specific SDA and SCL pins
    Wire.begin(SDA_PIN, SCL_PIN);

    // I2C MUTEX PROTECTION: Acquire the bus lock before initializing the sensor hardware
    // This ensures no other task (e.g., LCD task) interferes during sensor setup
    if (xSemaphoreTake(sharedData->i2cMutex, portMAX_DELAY) == pdTRUE) 
    {
        dht.begin();
        xSemaphoreGive(sharedData->i2cMutex); // Release lock immediately
    }

    // Main infinite loop for the sensor reading task
    while (1) 
    {
        // Local variables to hold temporary sensor readings
        float t = 0.0;
        float h = 0.0;
        bool readSuccess = false;

        // I2C MUTEX PROTECTION: The I2C bus must be locked before communicating with the DHT20
        if (xSemaphoreTake(sharedData->i2cMutex, pdMS_TO_TICKS(200)) == pdTRUE) 
        {
            // Execute the read command over I2C and check for a successful response
            if (dht.read() == DHT20_OK) 
            {
                t = dht.getTemperature();
                h = dht.getHumidity();
                readSuccess = true;
            }
            // Release the I2C mutex immediately after the physical read operation completes
            xSemaphoreGive(sharedData->i2cMutex); 
        }

        // Error handling: If reading failed or the I2C bus was too busy
        if (!readSuccess) 
        {
            Serial.println("Error: Failed to read DHT20 or I2C bus is busy!");
            vTaskDelay(pdMS_TO_TICKS(2000)); // Delay before retrying
            continue; // Skip the rest of the loop and try again
        }

        // DATA MUTEX: Securely update the shared memory with the new sensor readings
        // This prevents Race Conditions when the display task tries to read these variables
        if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            sharedData->temperature = t;
            sharedData->humidity = h;
            xSemaphoreGive(sharedData->mutex);

            // // SERIAL MUTEX PROTECTION: Prevent interleaved text output
            // if (xSemaphoreTake(sharedData->serialMutex, portMAX_DELAY) == pdTRUE) {
            //     Serial.printf("Temperature: %.2f°C | Humidity: %.2f%%\n", t, h);
            //     xSemaphoreGive(sharedData->serialMutex);
            // }
        }

        // Determine the current state and assign the corresponding string
        const char* stateStr = "";

        // THRESHOLD LOGIC: Trigger the appropriate state semaphore based on the temperature
        if (t >= 20 && t < 30) 
        {
            // Normal state
            xSemaphoreGive(sharedData->semNormal);
            stateStr = "Normal";
        } 
        else if ((t >= 30.0 && t < 38.0) || (t >= 7 && t < 20))
        {
            // Warning state
            xSemaphoreGive(sharedData->semWarning);
            stateStr = "Warning";
        } 
        else 
        {
            // Critical state
            xSemaphoreGive(sharedData->semCritical);
            stateStr = "Critical";
        }

        // SERIAL MUTEX PROTECTION: Combine output into a single atomic print and flush
        if (xSemaphoreTake(sharedData->serialMutex, portMAX_DELAY) == pdTRUE) 
        {
            Serial.printf("Temperature: %.2f°C | Humidity: %.2f%% | State: %s\n", t, h, stateStr);
            Serial.flush(); // Ensure the USB CDC buffer is completely sent to the host PC
            xSemaphoreGive(sharedData->serialMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}