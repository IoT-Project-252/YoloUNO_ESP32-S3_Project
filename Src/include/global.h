#ifndef GLOBAL_H
#define GLOBAL_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

// Gom tất cả dữ liệu cần chia sẻ và các công cụ đồng bộ (Semaphore/Mutex) vào một struct
struct SensorData 
{
    // Dữ liệu cảm biến
    float temperature;
    float humidity;
    
    // Mutex để bảo vệ biến nhiệt độ, độ ẩm khi các task truy cập
    SemaphoreHandle_t mutex;
    
    // Các Binary Semaphore để báo hiệu 3 trạng thái lên màn hình LCD
    SemaphoreHandle_t semNormal;
    SemaphoreHandle_t semWarning;
    SemaphoreHandle_t semCritical;
};

#endif