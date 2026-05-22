#include "led_blinky.h"

static const uint8_t LED_PWM_CH = 0;          // CH = Channel
static const uint32_t LED_PWM_FREQ = 5000;    // PWM frequence = 5000Hz
static const uint8_t LED_PWM_RES = 8;         // Resolution = 0..255 (8-bit)

void led_blinky(void *pvParameters){
  auto *sharedData = static_cast<SharedData*>(pvParameters);
  ledcSetup(LED_PWM_CH, LED_PWM_FREQ, LED_PWM_RES); // Config Channel with freq + res
  ledcAttachPin(LED_GPIO, LED_PWM_CH);    // Connect LED pin to PWM channel

  uint8_t mode = 1; // 0: <26, 1: 26-28, 2: >28
  
  while(1) {
    if (xSemaphoreTake(sharedData->semTempNormal, 0) == pdTRUE) {
      mode = 0;
    } else if (xSemaphoreTake(sharedData->semTempWarning, 0) == pdTRUE) {
      mode = 1;
    } else if (xSemaphoreTake(sharedData->semTempCritical, 0) == pdTRUE) {
      mode = 2;
    }

    uint8_t duty = 160;
    uint32_t on_ms = 500;
    uint32_t off_ms = 500;

    if (mode == 0) {
      duty = 64;
      on_ms = 1000;
      off_ms = 1000;
    } else if (mode == 1) {
      duty = 160;
      on_ms = 500;
      off_ms = 500;
    } else {
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