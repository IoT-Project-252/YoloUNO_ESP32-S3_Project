"""Run mock scenario tests against the exported TFLite model.

Usage:
  python mock_scenarios.py
"""
import os
import json
import typing
from collections import OrderedDict as _BaseOrderedDict

if not hasattr(typing, "OrderedDict"):
    class _OrderedDict(_BaseOrderedDict):
        @classmethod
        def __class_getitem__(cls, item):
            return cls

    typing.OrderedDict = _OrderedDict

import numpy as np
import tensorflow as tf

BASE = os.path.dirname(__file__)
OUT_DIR = os.path.join(BASE, "output")
TFLITE_PATH = os.path.join(OUT_DIR, "model_int8.tflite")
SCALER_PATH = os.path.join(OUT_DIR, "scaler_params.json")

LABELS = {
    0: "Normal",
    1: "TempAnomaly",
    2: "HumidityAnomaly",
}

SCENARIOS = [
    # Normal
    {"name": "Normal-1", "temp": 26.50, "humidity": 60.00, "expected": 0},
    {"name": "Normal-2", "temp": 29.00, "humidity": 55.00, "expected": 0},
    # Temperature anomaly high
    {"name": "TempHigh-1", "temp": 45.00, "humidity": 60.00, "expected": 1},
    {"name": "TempHigh-2", "temp": 50.00, "humidity": 65.00, "expected": 1},
    # Temperature anomaly low
    {"name": "TempLow-1", "temp": 5.00, "humidity": 55.00, "expected": 1},
    {"name": "TempLow-2", "temp": 8.00, "humidity": 70.00, "expected": 1},
    # Humidity anomaly high
    {"name": "HumHigh-1", "temp": 28.00, "humidity": 95.00, "expected": 2},
    {"name": "HumHigh-2", "temp": 24.00, "humidity": 98.00, "expected": 2},
    # Humidity anomaly low
    {"name": "HumLow-1", "temp": 26.00, "humidity": 10.00, "expected": 2},
    {"name": "HumLow-2", "temp": 30.00, "humidity": 15.00, "expected": 2},
    # Boundary tests
    {"name": "Boundary-N1", "temp": 22.00, "humidity": 45.00, "expected": 0},
    {"name": "Boundary-N2", "temp": 32.00, "humidity": 75.00, "expected": 0},
    {"name": "Boundary-TH", "temp": 60.00, "humidity": 60.00, "expected": 1},
    {"name": "Boundary-TL", "temp": -10.00, "humidity": 60.00, "expected": 1},
    {"name": "Boundary-HH", "temp": 25.00, "humidity": 100.00, "expected": 2},
    {"name": "Boundary-HL", "temp": 25.00, "humidity": 0.00, "expected": 2},
    # Mixed extremes (temp anomaly priority)
    {"name": "Mixed-1", "temp": 50.00, "humidity": 98.00, "expected": 1},
    {"name": "Mixed-2", "temp": 5.00, "humidity": 10.00, "expected": 1},
]


def load_scaler(path=SCALER_PATH):
    with open(path, "r") as f:
        return json.load(f)


def scale_inputs(temp_raw, humidity_raw, scaler_params):
    data_min = np.array(scaler_params["data_min"], dtype=np.float32)
    data_max = np.array(scaler_params["data_max"], dtype=np.float32)
    x = np.array([temp_raw, humidity_raw], dtype=np.float32)
    x = (x - data_min) / (data_max - data_min)
    x = np.clip(x, 0.0, 1.0)
    return x


def load_interpreter(path=TFLITE_PATH):
    interpreter = tf.lite.Interpreter(model_path=path)
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]
    return interpreter, input_details, output_details


def predict(interpreter, input_details, output_details, x_scaled):
    input_scale, input_zero = input_details["quantization"]
    output_scale, output_zero = output_details["quantization"]

    x_q = (x_scaled / input_scale + input_zero).astype(np.int8)
    interpreter.set_tensor(input_details["index"], x_q.reshape(1, -1))
    interpreter.invoke()

    out_q = interpreter.get_tensor(output_details["index"])  # int8
    out_f = (out_q.astype(np.float32) - output_zero) * output_scale
    pred = int(np.argmax(out_f, axis=1)[0])
    return pred, out_f.flatten()


def main():
    if not os.path.exists(TFLITE_PATH):
        raise FileNotFoundError("Missing model_int8.tflite. Run train_and_convert.py first.")
    if not os.path.exists(SCALER_PATH):
        raise FileNotFoundError("Missing scaler_params.json. Run train_and_convert.py first.")

    scaler_params = load_scaler()
    interpreter, input_details, output_details = load_interpreter()

    passed = 0
    print("Mock scenario results:\n")
    for s in SCENARIOS:
        x = scale_inputs(s["temp"], s["humidity"], scaler_params)
        pred, probs = predict(interpreter, input_details, output_details, x)
        ok = (pred == s["expected"])
        if ok:
            passed += 1

        print(
            f"{s['name']:<12} | temp={s['temp']:7.2f} hum={s['humidity']:7.2f} | "
            f"expected={LABELS[s['expected']]:<14} got={LABELS[pred]:<14} | "
            f"{'PASS' if ok else 'FAIL'}"
        )

    print("\nSummary:")
    print(f"Passed {passed}/{len(SCENARIOS)} scenarios")

if __name__ == "__main__":
    main()

