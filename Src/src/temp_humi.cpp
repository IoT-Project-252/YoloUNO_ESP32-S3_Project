#include "temp_humi.h"

void tem_humi(void *pvParameters)
{
    SharedData* sharedData = (SharedData*) pvParameters;

    DHT20 dht;

    Wire.begin(SDA_PIN, SCL_PIN);

    if (xSemaphoreTake(sharedData->i2cMutex, portMAX_DELAY) == pdTRUE) 
    {
        dht.begin();
        xSemaphoreGive(sharedData->i2cMutex);
    }

    while (1) 
    {
        float t = 0.0;
        float h = 0.0;
        bool readSuccess = false;

        // BỌC I2C MUTEX: Phải có khóa I2C mới được phép giao tiếp với DHT20
        if (xSemaphoreTake(sharedData->i2cMutex, pdMS_TO_TICKS(200)) == pdTRUE) 
        {
            if (dht.read() == DHT20_OK) 
            {
                t = dht.getTemperature();
                h = dht.getHumidity();
                readSuccess = true;
            }
            xSemaphoreGive(sharedData->i2cMutex); // Đọc xong phải trả khóa ngay
        }

        if (!readSuccess) 
        {
            Serial.println("Loi doc DHT20 hoac busy I2C!");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        // Cập nhật dữ liệu vào biến chung một cách an toàn
        if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            sharedData->temperature = t;
            sharedData->humidity = h;
            xSemaphoreGive(sharedData->mutex);
        }

        // Phát Semaphore trạng thái
        if (t < 30.0) 
        {
            xSemaphoreGive(sharedData->semNormal);
        } 
        else if (t >= 30.0 && t < 38.0) 
        {
            xSemaphoreGive(sharedData->semWarning);
        } 
        else 
        {
            xSemaphoreGive(sharedData->semCritical);
        }

        vTaskDelay(pdMS_TO_TICKS(2000)); 
    }
}