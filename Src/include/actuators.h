#ifndef ACTUATORS_H
#define ACTUATORS_H

#include "global.h"
#include <Adafruit_NeoPixel.h>

// ── GPIO Configuration ──────────────────────────────────────────────────────
#define NEOPIXEL_PIN    6     // GPIO for NeoPixel data line (D3)
#define NEOPIXEL_COUNT  4      // Number of NeoPixel LEDs

#define FAN_PIN         1     // GPIO for fan MOSFET / driver (A0)
#define FAN_PWM_CHANNEL 0      // LEDC channel
#define FAN_PWM_FREQ    25000  // 25 kHz – silent for most small fans
#define FAN_PWM_RES     8      // 8-bit resolution (0-255)

// ── FreeRTOS Task ───────────────────────────────────────────────────────────
void actuatorTask(void *pvParameters);

#endif
