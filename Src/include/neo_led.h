#ifndef NEO_LED_H
#define NEO_LED_H

#include "global.h"
#include <Adafruit_NeoPixel.h>

#define PIN_NEOPIXEL    45
#define LED_COUNT       1

void controlNeoLED(void *pvParameters);

#endif