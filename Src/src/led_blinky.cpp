#include "led_blinky.h"

static const uint8_t LED_PWM_CH = 0;          // CH = Channel
static const uint32_t LED_PWM_FREQ = 5000;    // PWM frequence = 5000Hz
static const uint8_t LED_PWM_RES = 8;         // Resolution = 0..255 (8-bit)

void led_blinky(void *pvParameters){
  auto *sharedData = static_cast<SharedData*>(pvParameters);
  ledcSetup(LED_PWM_CH, LED_PWM_FREQ, LED_PWM_RES); // Config Channel with freq + res
  ledcAttachPin(LED_GPIO, LED_PWM_CH);    // Connect LED pin to PWM channel

  float curr_temp = 25.0f;
  
  while(1) {
    if (sharedData != nullptr && sharedData->mutex != nullptr) {
      if (xSemaphoreTake(sharedData->mutex, 0) == pdTRUE) {
        curr_temp = sharedData->temperature;
        xSemaphoreGive(sharedData->mutex);
      }
    }

    uint8_t duty = 160;     // Default = normal
    uint32_t on_ms = 500;
    uint32_t off_ms = 500;

    if (curr_temp < 26.0f) {        // Temp low: slow speed
      duty = 64;
      on_ms = 1000;
      off_ms = 1000;
    }         
    else if (curr_temp <= 28.0f) {  // Temp normal: moderate speed
      duty = 160;
      on_ms = 500;
      off_ms = 500;
    }   
    else {      // Temp high: fast speed          
      duty = 255;
      on_ms = 200;
      off_ms = 200;
    }

    ledcWrite(LED_PWM_CH, duty);
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    ledcWrite(LED_PWM_CH, 0);
    vTaskDelay(pdMS_TO_TICKS(off_ms));
  }
}