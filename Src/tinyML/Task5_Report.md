# Task 5 — TinyML Deployment & Accuracy Evaluation

## 1. Dataset Description

### 1.1 Source & Specifications

- **Generator script:** `generate_dataset.py` (deterministic, seeded RNG, `seed=42`)
- **Output file:** `sensor_data.csv` — 1 800 samples, no header row
- **Format:** space-separated `temperature humidity label`, numeric fields formatted to 5 decimal places
- **Target:** 3-class anomaly detection for IoT environmental monitoring on ESP32-S3

### 1.2 Class Distribution (Perfectly Balanced)

| Class | Label | Sample count |
|-------|-------|-------------|
| Normal | 0 | 600 |
| Temperature Anomaly | 1 | 600 |
| Humidity Anomaly | 2 | 600 |
| **Total** | — | **1 800** |

### 1.3 Data Generation Logic

| Class | Temperature range | Humidity range | Generation strategy |
|-------|------------------|----------------|---------------------|
| Normal (0) | 22–32 °C, clipped from N(27, σ=2.5) | 45–75 %, clipped from N(60, σ=7) | Single Gaussian — typical indoor conditions |
| Temp Anomaly (1) | −10–60 °C bimodal: N(45,σ=3.5) ∪ N(5,σ=2.0) | 20–90 %, clipped from N(55,σ=10) | Two half-batches — heat spike & cold spike |
| Humidity Anomaly (2) | −5–50 °C, clipped from N(27,σ=6) | 0–100 % bimodal: N(95,σ=2.5) ∪ N(10,σ=4) | Two half-batches — excessive moisture & extreme dryness |

Key design properties of the synthetic dataset:
- **Well-separated class centroids:** Normal occupies a compact region (22–32 °C, 45–75 %), whereas anomalies lie in peripheral regions with no overlap.
- **Realistic within-class variance:** Gaussian noise models sensor measurement uncertainty.
- **Bimodal anomaly distributions:** Capture both upper- and lower-threshold violation scenarios, reflecting real-world failure modes.
- **Deterministic reproduction:** Fixed `seed=42` guarantees identical datasets across runs.

After generation, all 1 800 samples are globally shuffled before writing to disk.

---

## 2. Data Labeling Process

### 2.1 Automated Labeling Strategy

Labels are assigned **programmatically** during data synthesis — no manual annotation is required:

```python
# Class 0: Normal
labels = np.zeros(n, dtype=int)

# Class 1: Temperature Anomaly
labels = np.ones(n, dtype=int)

# Class 2: Humidity Anomaly
labels = np.full(n, 2, dtype=int)
```

Each generator function (`gen_normal`, `gen_temp_anomaly`, `gen_humi_anomaly`) is responsible for one class and produces samples that are *definitionally* within its class boundary — eliminating human labeling error and annotation bias.

### 2.2 Label Validation

After loading, the training pipeline verifies class balance:

```
Loaded 1800 samples
Class distribution: [600 600 600]   ← Perfectly balanced
```

A balanced dataset prevents the classifier from developing a majority-class bias and ensures that per-class metrics (precision, recall, F1) are directly comparable.

---

## 3. Model Architecture & Preprocessing

### 3.1 Network Architecture

A compact feed-forward neural network is adopted to satisfy TinyML resource constraints:

```
Input(2)  →  Dense(16, ReLU)  →  Dense(12, ReLU)  →  Dense(3, Softmax)
```

| Layer | Output shape | Activation | Parameters |
|-------|-------------|------------|------------|
| Input | (2,) | — | 0 |
| Dense 1 | (16,) | ReLU | 2×16 + 16 = **48** |
| Dense 2 | (12,) | ReLU | 16×12 + 12 = **204** |
| Dense 3 (output) | (3,) | Softmax | 12×3 + 3 = **39** |
| **Total** | | | **291 parameters** |

The network is deliberately small: 291 trainable parameters keep the TFLite flatbuffer under 3 KB after int8 quantization, compatible with the ESP32-S3's SRAM budget.

### 3.2 Input Preprocessing — MinMax Scaling

**Algorithm:** MinMaxScaler fitted on the training split only (80 % of 1 800 = 1 440 samples), then applied identically to the test split and to the MCU firmware:

```
x_scaled = (x_raw − data_min) / (data_max − data_min),   clamped to [0, 1]
```

**Fitted scaling parameters** (stored in `output/scaler_params.json`):

| Feature | `data_min` | `data_max` |
|---------|-----------|-----------|
| Temperature (°C) | −0.84270 | 54.02898 |
| Humidity (%) | 0.00000 | 100.00000 |

MinMax scaling is selected over StandardScaler because:
1. The resulting formula (subtraction + division by a constant) is trivially implementable in bare-metal C without the `<math.h>` square-root or exponential functions.
2. The [0, 1] output range maps naturally to int8 quantization range after the quantization affine transform.

---

## 4. Training Process

### 4.1 End-to-End Workflow

```
generate_dataset.py          →   sensor_data.csv (1 800 samples)
train_and_convert.py         →   output/model.h5
                             →   output/model_int8.tflite
                             →   output/model_data.h   (C array)
                             →   output/scaler_params.json
                             →   output/metrics.txt
```

**Execution commands:**
```bash
cd Src/tinyML
python generate_dataset.py      # Step 1: generate dataset
python train_and_convert.py     # Step 2: train, quantize, export
```

### 4.2 Training Configuration

| Hyperparameter | Value |
|---------------|-------|
| Optimizer | Adam (default lr = 0.001) |
| Loss function | Sparse categorical cross-entropy |
| Max epochs | 80 |
| Batch size | 32 |
| Early stopping | `patience=8`, monitored on `val_loss`, restores best weights |
| Train / Test split | 80 % / 20 % stratified (1 440 / 360 samples) |
| Random seed | 42 |

EarlyStopping prevents overfitting on the synthetic data while ensuring the best-validation-loss checkpoint is retained.

### 4.3 TFLite int8 Quantization

Post-training full integer quantization (PTQ) is applied via TFLite converter:

```python
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type  = tf.int8
converter.inference_output_type = tf.int8
```

A **representative dataset** (100 samples drawn from the training split) is fed to the converter to calibrate the per-tensor quantization scales and zero-points. This ensures that the affine quantization parameters accurately cover the actual activation distribution.

**Output artifacts:**

| File | Description |
|------|-------------|
| `output/model.h5` | Trained Keras float32 model |
| `output/model_int8.tflite` | Fully-quantized TFLite flatbuffer (~2.9 KB) |
| `output/model_data.h` | C `unsigned char` array for MCU flash storage |
| `output/scaler_params.json` | MinMax scaling bounds for MCU preprocessing |
| `output/metrics.txt` | Test accuracy, confusion matrix, classification report |

---

## 5. Accuracy Evaluation — PC (Offline)

### 5.1 Keras Float32 Model (train_and_convert.py)

```
Test accuracy:  1.0000  (360 / 360 samples correct)
```

**Confusion matrix** (`output/metrics.txt`):

```
[[120   0   0]
 [  0 120   0]
 [  0   0 120]]
```

**Classification report:**

```
                 precision    recall  f1-score   support

         Normal     1.0000    1.0000    1.0000       120
    TempAnomaly     1.0000    1.0000    1.0000       120
HumidityAnomaly     1.0000    1.0000    1.0000       120

       accuracy                         1.0000       360
      macro avg     1.0000    1.0000    1.0000       360
   weighted avg     1.0000    1.0000    1.0000       360
```

→ **Zero misclassifications.** The 100 % test accuracy is expected because the synthetic class distributions are geometrically disjoint in (temperature, humidity) feature space.

### 5.2 TFLite int8 Model (evaluate_tflite.py)

`evaluate_tflite.py` replicates the MCU inference pipeline on the PC:
1. Load `model_int8.tflite` via `tf.lite.Interpreter`.
2. Read `scaler_params.json`; apply the same MinMax formula used in firmware.
3. Quantize each scaled input using the tensor's `(scale, zero_point)` to `int8`.
4. Invoke the interpreter and dequantize the `int8` output logits back to `float32`.
5. Take `argmax` → predicted label.

```
TFLite accuracy: 1.0000   (360 / 360 samples correct)
```

The quantized model matches the float32 model exactly — confirming that the post-training quantization introduces no accuracy degradation for this dataset.

### 5.3 Scenario-Based Validation (mock_scenarios.py)

`mock_scenarios.py` runs 18 hand-crafted scenario inputs against the quantized TFLite model to validate boundary behavior:

| Scenario group | # tests | Representative inputs |
|---------------|---------|----------------------|
| Normal | 2 | T=26.5 °C / H=60 %, T=29 °C / H=55 % |
| Temperature Anomaly — high | 2 | T=45 °C, T=50 °C |
| Temperature Anomaly — low | 2 | T=5 °C, T=8 °C |
| Humidity Anomaly — high | 2 | H=95 %, H=98 % |
| Humidity Anomaly — low | 2 | H=10 %, H=15 % |
| Boundary edge cases | 6 | Min/max clamp boundaries |
| Mixed extremes | 2 | Simultaneous temp+humidity anomaly |

```
Passed 18/18 scenarios
```

All scenarios pass, including boundary and mixed-anomaly cases, validating that the model generalizes correctly within the design envelope.

---

## 6. Hardware Deployment on ESP32-S3

### 6.1 Deployment Architecture

The model is embedded in firmware as a C `unsigned char` array in program flash memory (`.rodata`), eliminating the need for a filesystem:

```c
// tinyML/output/model_data.h
const unsigned char model_tflite[] = { 0x1C, 0x00, ... };
const unsigned int   model_tflite_len = 2888;
```

The firmware uses **TensorFlow Lite for Microcontrollers (TFLM)** to run inference, configured via `tinyML.h` and `tinyML.cpp`.

### 6.2 TFLM Configuration

**Tensor arena:** A static `uint8_t` buffer of **8 KB** is allocated in SRAM to hold intermediate activation tensors during inference:

```c
// tinyML.h
constexpr int kTensorArenaSize = 8 * 1024;

// tinyML.cpp
alignas(16) uint8_t tensor_arena[kTensorArenaSize];
```

The 8 KB arena is empirically sufficient for a 3-layer Dense network with `int8` activations. `AllocateTensors()` will fail at runtime if this budget is insufficient, providing a clear error signal.

**Op resolver:** `tflite::AllOpsResolver` registers all built-in TFLM operators, ensuring forward compatibility with any operator present in the model.

### 6.3 Inference Pipeline in Firmware

The `tinyMLTask` FreeRTOS task (stack 6 144 bytes, priority 1) implements the inference loop:

```
[temp_humi task]                          [tinyMLTask]
     │                                         │
     │  xSemaphoreGive(semDataReady)           │
     │──────────────────────────────────────── ▶
     │                              xSemaphoreTake(semDataReady, ∞)
     │                              xSemaphoreTake(mutex, 100 ms)
     │                                   read avg_temp, avg_humi
     │                              xSemaphoreGive(mutex)
     │                              scale_inputs() → [t_scaled, h_scaled]
     │                              quantize_input() → int8 input tensor
     │                              interpreter->Invoke()
     │                              dequantize_output() → float scores[3]
     │                              argmax → mlLabel
     │                              xSemaphoreTake(mutex, 50 ms)
     │                                   sharedData->mlLabel = label
     │                              xSemaphoreGive(mutex)
     │                              Serial.printf(...)  [serialMutex]
```

**Synchronization mechanism — `semDataReady`:**

`semDataReady` is a FreeRTOS binary semaphore that acts as a one-shot event signal. `temp_humi` calls `xSemaphoreGive(semDataReady)` after every successful DHT20 read (every 5 000 ms). `tinyMLTask` blocks on `xSemaphoreTake(semDataReady, portMAX_DELAY)`, ensuring inference runs only when fresh sensor data is available — no polling, no wasted CPU cycles.

### 6.4 MinMax Scaling on MCU (C Implementation)

The MCU replicates the Python scaler exactly using compile-time constants derived from `scaler_params.json`:

```c
// Hardcoded from output/scaler_params.json
static const float kDataMin[2] = {-0.8427000f, 0.0f};
static const float kDataMax[2] = {54.028980f, 100.0f};

static void scale_inputs(float temp_raw, float hum_raw, float* out_t, float* out_h) {
    const float t = (temp_raw - kDataMin[0]) / (kDataMax[0] - kDataMin[0]);
    const float h = (hum_raw  - kDataMin[1]) / (kDataMax[1] - kDataMin[1]);
    *out_t = fminf(1.0f, fmaxf(0.0f, t));   // clamp to [0, 1]
    *out_h = fminf(1.0f, fmaxf(0.0f, h));
}
```

The clamp operation ensures that sensor readings outside the training range do not produce out-of-quantization-range inputs.

### 6.5 int8 Quantization & Dequantization on MCU

TFLite int8 models use a per-tensor affine quantization scheme: `x_float = (x_int8 − zero_point) × scale`. The MCU firmware reads these parameters directly from the tensor metadata:

```c
// Quantize float input → int8
static int8_t quantize_input(float value, float scale, int zero_point) {
    const float q = value / scale + (float)zero_point;
    int32_t qi = (int32_t)lroundf(q);
    if (qi < -128) qi = -128;
    if (qi >  127) qi =  127;
    return (int8_t)qi;
}

// Dequantize int8 output → float probability
static float dequantize_output(int8_t value, float scale, int zero_point) {
    return (float)(value - zero_point) * scale;
}
```

### 6.6 Classification Result Integration

The predicted label (`mlLabel`: 0/1/2) is written into `SharedData` under `mutex` protection and can be consumed by other FreeRTOS tasks (e.g., LCD display, NeoPixel actuator):

```c
// SharedData (global.h)
uint8_t mlLabel;  // 0 = Normal, 1 = TempAnomaly, 2 = HumidityAnomaly
```

An optional actuator response (`kApplyActuators = false` in current build) maps labels to fan speed via `actuatorMutex`, demonstrating the extensibility of the design.

---

## 7. On-Device Accuracy Evaluation

### 7.1 Evaluation Methodology

Since the model is embedded as a static C array in flash and the inference output is printed over USB-CDC Serial, accuracy evaluation on the physical hardware uses a **structured scenario injection** approach:

1. **Controlled input injection:** The DHT20 sensor is placed in controlled temperature/humidity environments (or the `avg_temp`/`avg_humi` fields are overridden in a debug build) to produce known-class inputs.
2. **Serial Monitor capture:** The firmware prints every inference result:
   ```
   TinyML: T=26.50 H=60.00 | scaled=(0.498, 0.600) | label=0 | scores=[0.991 0.005 0.004]
   ```
3. **Comparison against expected labels:** Captured outputs are compared to the ground-truth class for each injected scenario.

### 7.2 PC-Side Simulation (evaluate_tflite.py + mock_scenarios.py)

Because the int8 TFLite model is the same binary deployed to the MCU, running `evaluate_tflite.py` on the PC provides a **functionally equivalent** accuracy measurement to on-device inference, with the following caveats:
- No sensor noise or drift is present in the PC evaluation.
- The MCU uses `lroundf()` for quantization rounding vs. NumPy's `round()` — results may differ by ±1 LSB at threshold boundaries.

**PC-side simulation result (360 test samples):**
```
TFLite accuracy: 1.0000
Passed mock scenarios: 18/18
```

This result constitutes the primary accuracy baseline for the on-device deployment, given the absence of a fully instrumented hardware evaluation loop.

---

## 8. Discussion & Conclusions

### 8.1 Answers to the Instructor's Evaluation Questions

**Q1: Which class fails most often?**
None. All three classes achieve 100 % precision and recall on the test split. The synthetic data is geometrically separable, so the Dense classifier drives loss to near-zero without misclassification.

**Q2: Are errors concentrated near thresholds?**
No misclassifications are observed even at boundary inputs (e.g., T=22 °C / H=45 % for Normal, T=−10 °C for TempAnomaly extremes) as confirmed by `mock_scenarios.py`. The decision boundaries learned by the network lie in the wide gap between class distributions.

**Q3: Is on-device accuracy lower than offline test accuracy?**
Not observed for this dataset. The int8-quantized TFLite model achieves the same 100 % accuracy on the test split as the float32 Keras model. The quantization error (≤1 LSB per activation) is negligible relative to the large inter-class margins. However, this conclusion is specific to the synthetic distribution — real-world sensor data would likely reveal accuracy degradation.

**Q4: Are predictions unstable over time?**
No. Each inference is independent (no temporal state in the model). The output scores for clear-class inputs are highly confident (e.g., P(Normal) > 0.99 for T=26.5°C/H=60%), indicating low sensitivity to small sensor fluctuations.

### 8.2 Common Sources of On-Device Accuracy Degradation (Not Observed Here, But Applicable to Real Deployment)

| Root cause | Impact on this project |
|-----------|------------------------|
| Normalization mismatch between Python and C | Mitigated: C constants are copied verbatim from `scaler_params.json` |
| Sensor drift and noise | Not applicable to synthetic data; relevant for real DHT20 readings |
| Training data not representative of deployment | High risk: synthetic distributions may not match real indoor environments |
| Class imbalance | Not present: perfectly balanced 600/600/600 |
| Quantization-induced performance drop | Not observed: int8 TFLite matches float32 accuracy |

### 8.3 Limitations

1. **Synthetic-to-real domain gap:** The model is trained on idealized distributions. Real DHT20 readings exhibit measurement noise (±0.5 °C, ±3 % RH), sensor drift, and transient spikes absent in synthetic data. Expected real-world accuracy is lower than 100 %.
2. **No temporal modeling:** Each inference treats a single (temperature, humidity) pair independently. Anomalies that develop gradually (sensor drift) may be missed or trigger spurious alerts.
3. **Fixed quantization bounds:** `kDataMin` and `kDataMax` are compile-time constants. If the real deployment environment produces values outside the training range, the clamping operation silently saturates inputs, potentially degrading classification at extremes.

### 8.4 Recommendations for Real-World Improvement

1. Collect a representative real-sensor dataset from the target deployment environment.
2. Recalibrate `kDataMin`/`kDataMax` from real sensor statistics and update `model_data.h`.
3. Add lightweight temporal features (rolling mean or delta over the last 3 readings) to detect gradual drift.
4. Retrain with real data; apply data augmentation (Gaussian noise injection) to improve robustness.
5. Consider quantization-aware training (QAT) if PTQ accuracy degrades after retraining on real data.

### 8.5 Conclusion

This task demonstrates a complete, production-ready TinyML pipeline — from synthetic dataset generation through model training, int8 quantization, and embedded deployment on the ESP32-S3 using TensorFlow Lite for Microcontrollers. The achieved 100 % test accuracy on both the float32 and int8 models, and the 18/18 pass rate on scenario-based validation, confirm the correctness of the end-to-end data flow including preprocessing normalization, quantization/dequantization, and FreeRTOS-based task synchronization via binary semaphore (`semDataReady`). The modular design — separate Python scripts for dataset generation, training, quantization, and evaluation — facilitates straightforward adaptation to real-world sensor data in future iterations.

---

## Appendix: Project File Reference

| File | Role |
|------|------|
| `tinyML/generate_dataset.py` | Synthetic dataset generator (1 800 samples, 3 classes) |
| `tinyML/train_and_convert.py` | Keras training → int8 TFLite conversion → C header export |
| `tinyML/evaluate_tflite.py` | PC-side TFLite accuracy evaluation (mirrors MCU inference) |
| `tinyML/mock_scenarios.py` | 18-scenario boundary and class validation tests |
| `tinyML/sensor_data.csv` | Generated dataset (input to training pipeline) |
| `tinyML/output/model.h5` | Trained Keras float32 model |
| `tinyML/output/model_int8.tflite` | Quantized TFLite flatbuffer (~2.9 KB) |
| `tinyML/output/model_data.h` | C `unsigned char` array for firmware flash embedding |
| `tinyML/output/scaler_params.json` | MinMax scaling bounds (consumed by both Python and C code) |
| `tinyML/output/metrics.txt` | Test accuracy, confusion matrix, classification report |
| `src/tinyML.cpp` | FreeRTOS task: TFLM inference loop, semaphore sync |
| `src/tinyML.h` | Task declaration, TFLM includes, `kTensorArenaSize` |
