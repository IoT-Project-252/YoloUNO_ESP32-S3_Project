#ifndef TINY_ML_H
#define TINY_ML_H

#include "global.h"
#include "temp_humi.h"

#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Size of Tensor Arena (configurable)
constexpr int kTensorArenaSize = 8 * 1024;

bool initTinyML();
void tinyMLTask(void *pvParameters);

#endif 