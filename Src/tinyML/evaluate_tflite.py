"""Evaluate the exported TFLite model on the test split.

Usage:
  python evaluate_tflite.py
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
import pandas as pd
from sklearn.model_selection import train_test_split
from sklearn.metrics import confusion_matrix, classification_report, accuracy_score
import tensorflow as tf


BASE = os.path.dirname(__file__)
CSV = os.path.join(BASE, "sensor_data.csv")
OUT_DIR = os.path.join(BASE, "output")
TFLITE_PATH = os.path.join(OUT_DIR, "model_int8.tflite")
SCALER_PATH = os.path.join(OUT_DIR, "scaler_params.json")


def load_data(path=CSV):
    data = pd.read_csv(path, sep=r"\s+", names=["temp", "humidity", "label"], engine="python")
    X = data[["temp", "humidity"]].values.astype(np.float32)
    y = data["label"].values.astype(np.int32)
    return X, y


def scale_inputs(X, scaler_params):
    data_min = np.array(scaler_params["data_min"], dtype=np.float32)
    data_max = np.array(scaler_params["data_max"], dtype=np.float32)
    X_scaled = (X - data_min) / (data_max - data_min)
    return np.clip(X_scaled, 0.0, 1.0)


def main():
    if not os.path.exists(TFLITE_PATH):
        raise FileNotFoundError("Missing model_int8.tflite. Run train_and_convert.py first.")

    with open(SCALER_PATH, "r") as f:
        scaler_params = json.load(f)

    X, y = load_data()
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y)

    X_test_scaled = scale_inputs(X_test, scaler_params)

    interpreter = tf.lite.Interpreter(model_path=TFLITE_PATH)
    interpreter.allocate_tensors()
    input_details = interpreter.get_input_details()[0]
    output_details = interpreter.get_output_details()[0]

    input_scale, input_zero = input_details["quantization"]
    output_scale, output_zero = output_details["quantization"]

    y_pred = []
    for i in range(X_test_scaled.shape[0]):
        x = X_test_scaled[i]
        x_q = (x / input_scale + input_zero).astype(np.int8)
        interpreter.set_tensor(input_details["index"], x_q.reshape(1, -1))
        interpreter.invoke()
        out_q = interpreter.get_tensor(output_details["index"])  # int8
        out_f = (out_q.astype(np.float32) - output_zero) * output_scale
        y_pred.append(int(np.argmax(out_f, axis=1)[0]))

    y_pred = np.array(y_pred, dtype=np.int32)
    acc = accuracy_score(y_test, y_pred)
    cm = confusion_matrix(y_test, y_pred)
    report = classification_report(
        y_test,
        y_pred,
        target_names=["Normal", "TempAnomaly", "HumidityAnomaly"],
        digits=4
    )

    print(f"TFLite accuracy: {acc:.4f}")
    print("Confusion matrix:")
    print(cm)
    print("Classification report:")
    print(report)


if __name__ == "__main__":
    main()
