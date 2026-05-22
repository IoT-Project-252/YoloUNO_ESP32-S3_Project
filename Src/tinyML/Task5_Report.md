# Task 5 — TinyML Deployment & Accuracy Evaluation

## 1 Dataset Description

**Source & Specifications:**
- Synthetic dataset generated using `generate_dataset.py` 
- File: `sensor_data.csv` (1800 samples, no header)
- Format: space-separated `temperature humidity label` with 5 decimals for numeric fields

**Class Distribution (Balanced):**
- Class 0 (Normal): 600 samples
- Class 1 (Temperature Anomaly): 600 samples  
- Class 2 (Humidity Anomaly): 600 samples

**Data Generation Logic:**

| Class | Temperature | Humidity | Distribution | Rationale |
|-------|-------------|----------|--------------|-----------|
| Normal | 22–32°C | 45–75% | N(27°, σ=2.5) / N(60%, σ=7) | Typical room conditions |
| Temp Anomaly | −10–60°C (bimodal) | 20–90% | Half ~45°C, half ~5°C | Heat spikes / cold spikes |
| Humidity Anomaly | −5–50°C | 0–100% (bimodal) | Half ~95%, half ~10% | Excessive moisture / dryness |

The synthetic distributions mimic realistic sensor behavior with:
- Well-separated class centroids (distinguishable by the model)
- Plausible within-class variance (small Gaussian noise)
- No overlap between Normal and anomaly classes (ensures perfect separability)

## 2 Data Labeling Process

**Automated Labeling:**
- Labels are assigned **deterministically** during synthetic data generation
- Each sample is created by the appropriate generator function for its class
- No manual labeling required → **eliminates human error and bias**

**Validation:**
```
Loaded 1800 samples
Class distribution: [600 600 600]  ← Perfectly balanced
```

## 3 Model Architecture & Preprocessing

**Model Type (TinyML-ready):**
- Small Dense neural network (Keras)
- Layers: Input(2) → Dense(16, ReLU) → Dense(12, ReLU) → Dense(3, Softmax)
- Lightweight for MCU deployment (few parameters)

**Input Preprocessing:**
- **MinMaxScaler**: scales each feature to [0.0, 1.0]
- Scaling parameters saved in `output/scaler_params.json`

**Key Design Decisions:**
- MinMax scaling is simple to implement in C (subtraction + division)
- Network is small enough for TFLite Micro
- Quantization to int8 for lower memory use

## 4 Training Process

**Workflow:**
1. Load 1800 samples from `sensor_data.csv`
2. Split: 80% train (1440), 20% test (360) stratified by class
3. Fit MinMaxScaler on training set
4. Train Dense neural network
5. Evaluate on scaled test data
6. Convert to TFLite int8 and export C header

**Code Execution:**
```bash
cd tinyML
python generate_dataset.py      # Generate sensor_data.csv
python train_and_convert.py     # Train model & export artifacts
```

**Output Artifacts:**
- `output/model.h5` — trained Keras model
- `output/model_int8.tflite` — quantized TFLite model
- `output/model_data.h` — C array for MCU
- `output/scaler_params.json` — scaling parameters
- `output/metrics.txt` — accuracy, confusion matrix, report

## 5 Accuracy Evaluation (PC Results)

**Training & Test Accuracy:**
```
Train accuracy: 1.0000 (100%)
Test accuracy:  1.0000 (100%)
```

**Confusion Matrix:**
```
                 Predicted
                 Normal  TempAnom  HumidAnom
Actual Normal      120        0          0
       TempAnom      0      120          0  
       HumidAnom     0        0        120
```
→ **Zero misclassifications** (all 360 test samples correctly predicted)

**Classification Report:**
```
                precision    recall  f1-score   support
        Normal      1.0000    1.0000    1.0000       120
    TempAnomaly    1.0000    1.0000    1.0000       120
HumidityAnomaly    1.0000    1.0000    1.0000       120

  weighted avg      1.0000    1.0000    1.0000       360
```

**Analysis:**
- Perfect precision (no false positives)
- Perfect recall (no false negatives)
- Perfect F1-score (balanced performance)

The high accuracy (100%) is expected because:
1. Synthetic data is highly separable (minimal overlap)
2. The Dense network has enough capacity for this 2D classification task
3. Only 2 input features → low-dimensional, easy to split

**TFLite Check (PC as hardware proxy):**
- Run `evaluate_tflite.py` to validate the exported int8 model on the same test split.
- This simulates MCU inference using TFLite and reports accuracy and confusion matrix.

## 6 Hardware Deployment Guide

### 6.1 Deployment Strategy (TFLite Micro)

The model is exported as an int8 TFLite model, then converted to a C array.
Deployment steps:

1. Include the generated header `model_data.h` in firmware.
2. Use TensorFlow Lite Micro to load and run the model:
   - `tflite::GetModel(model_tflite)`
   - `tflite::MicroInterpreter` with a static tensor arena
3. Apply MinMax scaling using values from `scaler_params.json`.

### 6.2 Scaling on MCU (C Implementation)

Use the same formula as training:
```c
temp_scaled = (temp_raw - data_min_temp) / (data_max_temp - data_min_temp);
humidity_scaled = (hum_raw - data_min_hum) / (data_max_hum - data_min_hum);
```
Clamp both to [0, 1] before copying into input tensor.

### 6.3 Integration Steps

1. Copy `model_data.h` into your firmware project
2. Add TFLite Micro sources and include paths
3. Implement inference loop:
```c
// 1) Read sensor values
// 2) Apply MinMax scaling
// 3) Write into input tensor
// 4) Invoke interpreter
// 5) Read output probabilities
```

## 7 Hardware Accuracy Evaluation (Without Real Hardware)

**Simulation Approach:**

Since lab may not have deployed hardware, simulate inference:

```python
# test_inference_simulation.py
import pickle
import numpy as np
from sklearn.preprocessing import MinMaxScaler
import json

# Load trained model and scaler
with open("output/model.pkl", "rb") as f:
    model = pickle.load(f)

with open("output/scaler_params.json", "r") as f:
    params = json.load(f)

# Simulate MCU inference
def mcu_predict_simulated(temp_raw, humidity_raw, model, params):
    # Apply scaling (exactly as C code would do)
    temp_s = (temp_raw - params["data_min"][0]) / \
             (params["data_max"][0] - params["data_min"][0])
    humidity_s = (humidity_raw - params["data_min"][1]) / \
                 (params["data_max"][1] - params["data_min"][1])
    
    # Clamp
    temp_s = np.clip(temp_s, 0.0, 1.0)
    humidity_s = np.clip(humidity_s, 0.0, 1.0)
    
    # Predict (simulating MCU inference)
    return model.predict([[temp_s, humidity_s]])[0]

# Test on held-out set
from sklearn.model_selection import train_test_split
X, y = ..., ...
X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

# Run "simulated MCU" on 100 test samples
y_mcu_sim = [mcu_predict_simulated(X_test[i, 0], X_test[i, 1], model, params) 
             for i in range(100)]
y_expected = y_test[:100]

accuracy_sim = np.mean(y_mcu_sim == y_expected)
print(f"Simulated MCU Accuracy: {accuracy_sim:.4f}")
```

**Expected Result:** Accuracy matches PC (100%) due to identical scaling & model logic.

## 8 Discussion & Conclusions

### 8.1 Key Achievements

✓ Successfully generated **balanced synthetic dataset** (1800 samples, 600/class)  
✓ Implemented **realistic data generation logic** with class-specific distributions  
✓ Trained **lightweight Dense neural network** (TinyML-ready)  
✓ Achieved **perfect classification accuracy** (100% on test set)  
✓ Exported **deployment-ready artifacts**:
  - Keras model (`model.h5`)
  - Quantized TFLite model (`model_int8.tflite`)
  - C model array (`model_data.h`)
  - Scaling parameters (`scaler_params.json`)
  - Metrics (`metrics.txt`)

### 8.2 Strengths of Approach

1. **Rapid Development:** Synthetic data generation avoids months of sensor collection
2. **Reproducibility:** Deterministic class labels and seeded RNG ensure consistent results
3. **Hardware-Friendly:** Small Dense network + int8 quantization are MCU-suitable
4. **Simplicity:** Only 2 input features, minimal preprocessing
5. **Perfect Test Accuracy:** Model is production-ready for the synthetic domain

### 8.3 Limitations & Future Work

**Limitations:**
- Synthetic data is overly separable → may overestimate real-world accuracy
- Quantized Dense model still requires floating point scaling; for ultra-low-power MCU, consider fixed-point scaling
- No temporal patterns: treats each sensor reading independently

**For Real Deployment:**
1. Collect real sensor data from target environment
2. Recalibrate scaler using actual sensor ranges
3. Retrain model with real data (likely lower accuracy, higher variance)
4. Test model robustness to sensor drift and noise
5. Consider time-series models (LSTM/GRU) if anomalies are temporal

**To Improve MCU Performance:**
- Use **int8 quantization** (already applied)
- Profile execution time on actual hardware (STM32/ESP32)
- Reduce Dense layer sizes if memory is constrained

### 8.4 Conclusion

This Task 5 demonstrates a **complete TinyML workflow** from synthetic data generation → model training → hardware-ready deployment. The achieved 100% test accuracy on synthetic data provides a strong baseline for IoT anomaly detection applications. The modular approach (separate dataset generator, trainer, inference template) allows easy integration into embedded firmware and future extension to real-world sensor data.

---

## Project Files

| File | Purpose |
|------|---------|
| `generate_dataset.py` | Create synthetic sensor dataset |
| `train_and_convert.py` | Train model & export artifacts |
| `sensor_data.csv` | Input dataset (1800 samples) |
| `evaluate_tflite.py` | Validate TFLite accuracy on test split |
| `output/model.h5` | Trained Keras model |
| `output/model_int8.tflite` | Quantized TFLite model |
| `output/model_data.h` | C model array for MCU |
| `output/scaler_params.json` | Scaling parameters |
| `output/metrics.txt` | Accuracy & confusion matrix |
