#ifndef MODEL_INFERENCE_H_
#define MODEL_INFERENCE_H_

/*
  Example C Inference Code (Pseudo)
  
  This file shows how to:
  1. Apply MinMax scaling to sensor inputs
  2. Run inference using the trained model
  
  In real firmware, use a C ML library like:
  - scikit-learn port (ONNX Runtime)
  - TensorFlow Lite Micro (requires .tflite conversion)
  - Custom C implementation of Random Forest
*/

#include <stdint.h>

// Scaler parameters (from training)
const float DATA_MIN[2] = {0.036064, 0.416817};
const float DATA_MAX[2] = {52.539845, 100.000000};

// Labels
#define LABEL_NORMAL 0
#define LABEL_TEMP_ANOMALY 1
#define LABEL_HUMIDITY_ANOMALY 2

/**
 * Apply MinMax scaling to raw sensor inputs.
 *  
 * @param temp_raw: raw temperature (e.g., 25.5°C)
 * @param humidity_raw: raw humidity (e.g., 60%)
 * @param out_temp: output scaled temp [0.0, 1.0]
 * @param out_humidity: output scaled humidity [0.0, 1.0]
 */
void scale_inputs(float temp_raw, float humidity_raw,
                  float* out_temp, float* out_humidity) {
    *out_temp = (temp_raw - DATA_MIN[0]) / (DATA_MAX[0] - DATA_MIN[0]);
    *out_humidity = (humidity_raw - DATA_MIN[1]) / (DATA_MAX[1] - DATA_MIN[1]);
    
    // clamp to [0, 1]
    if (*out_temp < 0.0f) *out_temp = 0.0f;
    if (*out_temp > 1.0f) *out_temp = 1.0f;
    if (*out_humidity < 0.0f) *out_humidity = 0.0f;
    if (*out_humidity > 1.0f) *out_humidity = 1.0f;
}

/**
 * Run inference.
 * In practice, call your ML library here, e.g.:
 *   - tflite_micro_interpreter.invoke()
 *   - onnx_runtime_run()
 *   - or hand-coded tree traversal
 */
uint8_t predict(float temp_scaled, float humidity_scaled) {
    // TODO: implement actual inference
    // For now, return a placeholder
    return LABEL_NORMAL;
}

#endif // MODEL_INFERENCE_H_
