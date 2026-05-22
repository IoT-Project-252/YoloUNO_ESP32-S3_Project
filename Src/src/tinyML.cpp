#include "tinyML.h"
#include <math.h>

#include "../tinyML/output/model_data.h"

// Declare global variables for TFlite
namespace {
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;

    // Allocate memory for Tensor Arena (static array to avoid heap fragmentation)
    alignas(16) uint8_t tensor_arena[kTensorArenaSize];
}

// Scaler params from tinyML/output/scaler_params.json
static const float kDataMin[2] = {-0.8427000f, 0.0f};
static const float kDataMax[2] = {54.028980f, 100.0f};

static void scale_inputs(float temp_raw, float hum_raw, float* out_t, float* out_h)
{
    const float t = (temp_raw - kDataMin[0]) / (kDataMax[0] - kDataMin[0]);
    const float h = (hum_raw - kDataMin[1]) / (kDataMax[1] - kDataMin[1]);
    *out_t = fminf(1.0f, fmaxf(0.0f, t));
    *out_h = fminf(1.0f, fmaxf(0.0f, h));
}

static int8_t quantize_input(float value, float scale, int zero_point)
{
    const float q = value / scale + (float)zero_point;
    int32_t qi = (int32_t)lroundf(q);
    if (qi < -128) qi = -128;
    if (qi > 127) qi = 127;
    return (int8_t)qi;
}

static float dequantize_output(int8_t value, float scale, int zero_point)
{
    return (float)(value - zero_point) * scale;
}

static void apply_actuators(SharedData* sd, uint8_t label)
{
    bool fan_on = false;
    uint8_t fan_speed = 0;

    if (label == 0) {
        fan_on = false; fan_speed = 0;
    } else if (label == 1) {
        fan_on = true; fan_speed = 100;
    } else {
        fan_on = true; fan_speed = 50;
    }

    if (xSemaphoreTake(sd->actuatorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        sd->fanOn = fan_on;
        sd->fanSpeed = fan_speed;
        sd->fanChanged = true;
        xSemaphoreGive(sd->actuatorMutex);
    }
}

// Interpreter initialization function
bool initTinyML() {
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(model_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        TF_LITE_REPORT_ERROR(error_reporter, "Model schema %d != supported %d",
                             model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    static tflite::AllOpsResolver resolver;
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
        return false;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    if (input == nullptr || output == nullptr) {
        TF_LITE_REPORT_ERROR(error_reporter, "Input/output tensors not found");
        return false;
    }

    Serial.println("TinyML Init: Model loaded.");
    return true;
}

// Task handle Inference
void tinyMLTask(void* pvParameters) {
    SharedData* sharedData = (SharedData*) pvParameters;

    if (!initTinyML()) {
        Serial.println("TinyML Task: Initialization failed. Task suspended.");
        vTaskSuspend(NULL);
    }

    uint8_t last_label = 255;

    while (1)
    {
        if (xSemaphoreTake(sharedData->semDataReady, portMAX_DELAY) == pdTRUE) {
            float curr_t = 0.0f;
            float curr_h = 0.0f;

            if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                curr_t = sharedData->avg_temp;
                curr_h = sharedData->avg_humi;
                xSemaphoreGive(sharedData->mutex);
            } else {
                continue;
            }

            float t_scaled = 0.0f;
            float h_scaled = 0.0f;
            scale_inputs(curr_t, curr_h, &t_scaled, &h_scaled);

            const float in_scale = input->params.scale;
            const int in_zero = input->params.zero_point;
            input->data.int8[0] = quantize_input(t_scaled, in_scale, in_zero);
            input->data.int8[1] = quantize_input(h_scaled, in_scale, in_zero);

            if (interpreter->Invoke() != kTfLiteOk) {
                Serial.println("TinyML Task: Invoke() failed");
                continue;
            }

            const float out_scale = output->params.scale;
            const int out_zero = output->params.zero_point;

            float scores[3] = {0};
            int best = 0;
            for (int i = 0; i < 3; i++) {
                scores[i] = dequantize_output(output->data.int8[i], out_scale, out_zero);
                if (scores[i] > scores[best]) {
                    best = i;
                }
            }

            const uint8_t label = (uint8_t)best;
            if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                sharedData->mlLabel = label;
                xSemaphoreGive(sharedData->mutex);
            }

            if (label != last_label) {
                apply_actuators(sharedData, label);
                last_label = label;
            }

            if (xSemaphoreTake(sharedData->serialMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                Serial.printf("TinyML: T=%.2f H=%.2f | scaled=(%.3f, %.3f) | label=%u | scores=[%.3f %.3f %.3f]\n",
                              curr_t, curr_h, t_scaled, h_scaled, label, scores[0], scores[1], scores[2]);
                Serial.flush();
                xSemaphoreGive(sharedData->serialMutex);
            }
        }
    }
}