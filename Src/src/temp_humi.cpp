#include "temp_humi.h"
#include <math.h>

#define WINDOW_SIZE 10

void temp_humi(void *pvParameters)
{
    // Cast the generic task parameter back to the SharedData structure pointer
    SharedData* sharedData = (SharedData*) pvParameters;

    // Instantiate the DHT20 sensor object
    DHT20 dht;

    // Initialize the I2C bus with specific SDA and SCL pins
    Wire.begin(SDA_PIN, SCL_PIN);

    // I2C MUTEX PROTECTION: Acquire the bus lock before initializing 
    if (xSemaphoreTake(sharedData->i2cMutex, portMAX_DELAY) == pdTRUE) 
    {
        dht.begin();
        xSemaphoreGive(sharedData->i2cMutex); // Release lock immediately
    }

    // Init Ring buffer
    float temp_buffer[WINDOW_SIZE] = {0};
    float humi_buffer[WINDOW_SIZE] = {0};
    int buffer_idx = 0;
    int valid_sample = 0; // Sample for first 10s

#ifdef CSV_LOGGER_MODE
    Serial.println("timestamp,temp_c,humi_pct");
#endif

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

        // VALIDATION: Reject invalid samples (NaN or physically impossible bounds)
        if (!readSuccess || isnan(t) || isnan(h) || t < -10.0 || t > 80.0 || h < 0.0 || h > 100) 
        {
#ifndef CSV_LOGGER_MODE
            Serial.println("Error: Invalid reading (NaN/Out-of-range) or I2C bus is busy!");
#endif
            vTaskDelay(pdMS_TO_TICKS(1000)); // Delay before retrying
            continue; // Skip the rest of the loop and try again
        }

        // Update ring buffer for moving average
        temp_buffer[buffer_idx] = t;
        humi_buffer[buffer_idx] = h;
        buffer_idx = (buffer_idx + 1) % WINDOW_SIZE;
        if (valid_sample < WINDOW_SIZE) {
            valid_sample++;
        }

        float sum_t = 0.0f;
        float sum_h = 0.0f;
        for (int i = 0; i < valid_sample; i++) {
            sum_t += temp_buffer[i];
            sum_h += humi_buffer[i];
        }
        float avg_t = sum_t / valid_sample;
        float avg_h = sum_h / valid_sample;

        // DATA MUTEX: Securely update the shared memory with the new sensor readings
        // This prevents Race Conditions when the display task tries to read these variables
        if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            sharedData->temperature = t;
            sharedData->humidity = h;
            sharedData->avg_temp = avg_t;
            sharedData->avg_humi = avg_h;
            xSemaphoreGive(sharedData->mutex);
        }

        // Signal TinyML task that new data is ready
        xSemaphoreGive(sharedData->semDataReady);

        // Task 1 + Task 3: Temperature state semaphores
        if (t < 26.0f) {
            xSemaphoreGive(sharedData->semTempNormal);
        } else if (t <= 28.0f) {
            xSemaphoreGive(sharedData->semTempWarning);
        } else {
            xSemaphoreGive(sharedData->semTempCritical);
        }

        // SERIAL MUTEX PROTECTION: Combine output into a single atomic print and flush
        if (xSemaphoreTake(sharedData->serialMutex, portMAX_DELAY) == pdTRUE) 
        {
            Serial.printf("Temperature: %.2f°C | Humidity: %.2f%% | AvgT: %.2f | AvgH: %.2f\n",
                          t, h, avg_t, avg_h);
            Serial.flush();
            xSemaphoreGive(sharedData->serialMutex);
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); 
    }
}