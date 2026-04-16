#ifndef TEMP_HUMI_H
#define TEMP_HUMI_H

#include "global.h"
#include "DHT20.h"
#include <Wire.h>

#define SDA_PIN 11
#define SCL_PIN 12

void temp_humi(void *pvParameters);

#endif