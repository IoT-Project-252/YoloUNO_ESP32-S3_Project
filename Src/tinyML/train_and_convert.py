"""Train a small 3-class Keras model and export .h5, .tflite, and C header.

Usage:
  python train_and_convert.py

This script expects the file sensor_data.csv in the same folder (generated
by generate_dataset.py). The file is space-separated with 5 decimals.
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
from sklearn.preprocessing import MinMaxScaler
from sklearn.metrics import confusion_matrix, classification_report
import tensorflow as tf


BASE = os.path.dirname(__file__)
CSV = os.path.join(BASE, "sensor_data.csv")
OUT_DIR = os.path.join(BASE, "output")
os.makedirs(OUT_DIR, exist_ok=True)


def load_data(path=CSV):
    # file has no header: columns temp, humidity, label (space-separated)
    data = pd.read_csv(path, sep=r"\s+", names=["temp", "humidity", "label"], engine="python")
    X = data[["temp", "humidity"]].values.astype(np.float32)
    y = data["label"].values.astype(np.int32)
    return X, y


def build_model(input_shape=(2,)):
    model = tf.keras.Sequential([
        tf.keras.layers.Input(shape=input_shape),
        tf.keras.layers.Dense(16, activation="relu"),
        tf.keras.layers.Dense(12, activation="relu"),
        tf.keras.layers.Dense(3, activation="softmax")
    ])
    model.compile(
        optimizer="adam",
        loss="sparse_categorical_crossentropy",
        metrics=["accuracy"]
    )
    return model


def representative_data_gen(X_train_scaled):
    for i in range(min(100, X_train_scaled.shape[0])):
        yield [np.expand_dims(X_train_scaled[i], axis=0).astype(np.float32)]


def save_scaler_params(scaler, path):
    params = {
        "data_min": scaler.data_min_.tolist(),
        "data_max": scaler.data_max_.tolist(),
        "feature_range": list(scaler.feature_range)
    }
    with open(path, "w") as f:
        json.dump(params, f, indent=2)


def write_c_header(tflite_model_path, header_path, array_name="model_tflite"):
    with open(tflite_model_path, "rb") as f:
        b = f.read()

    with open(header_path, "w") as h:
        h.write("#ifndef TFLITE_MODEL_H_\n")
        h.write("#define TFLITE_MODEL_H_\n\n")
        h.write("#include <stdint.h>\n\n")
        h.write(f"const unsigned char {array_name}[] = {{\n")
        for i in range(0, len(b), 12):
            row = b[i:i + 12]
            h.write("    " + ", ".join(str(int(x)) for x in row))
            if i + 12 < len(b):
                h.write(",\n")
            else:
                h.write("\n")
        h.write("};\n\n")
        h.write(f"const unsigned int {array_name}_len = {len(b)};\n\n")
        h.write("#endif // TFLITE_MODEL_H_\n")


def main():
    X, y = load_data()
    print("Loaded", X.shape[0], "samples")
    print("Class distribution:", np.bincount(y))

    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, random_state=42, stratify=y)

    scaler = MinMaxScaler(feature_range=(0.0, 1.0))
    X_train_scaled = scaler.fit_transform(X_train)
    X_test_scaled = scaler.transform(X_test)

    model = build_model(input_shape=(2,))
    callbacks = [
        tf.keras.callbacks.EarlyStopping(monitor="val_loss", patience=8, restore_best_weights=True)
    ]
    model.fit(
        X_train_scaled,
        y_train,
        epochs=80,
        batch_size=32,
        validation_data=(X_test_scaled, y_test),
        callbacks=callbacks
    )

    loss, acc = model.evaluate(X_test_scaled, y_test, verbose=0)
    print(f"Test accuracy: {acc:.4f}")

    y_pred = np.argmax(model.predict(X_test_scaled), axis=1)
    cm = confusion_matrix(y_test, y_pred)
    report = classification_report(
        y_test,
        y_pred,
        target_names=["Normal", "TempAnomaly", "HumidityAnomaly"],
        digits=4
    )

    print("Confusion matrix:")
    print(cm)
    print("Classification report:")
    print(report)

    metrics_path = os.path.join(OUT_DIR, "metrics.txt")
    with open(metrics_path, "w") as f:
        f.write(f"Test accuracy: {acc:.4f}\n\n")
        f.write("Confusion matrix:\n")
        f.write(np.array2string(cm))
        f.write("\n\nClassification report:\n")
        f.write(report)

    h5_path = os.path.join(OUT_DIR, "model.h5")
    model.save(h5_path)

    scaler_path = os.path.join(OUT_DIR, "scaler_params.json")
    save_scaler_params(scaler, scaler_path)

    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    converter.optimizations = [tf.lite.Optimize.DEFAULT]
    converter.representative_dataset = lambda: representative_data_gen(X_train_scaled)
    converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
    converter.inference_input_type = tf.int8
    converter.inference_output_type = tf.int8

    tflite_model = converter.convert()
    tflite_path = os.path.join(OUT_DIR, "model_int8.tflite")
    with open(tflite_path, "wb") as f:
        f.write(tflite_model)

    header_path = os.path.join(OUT_DIR, "model_data.h")
    write_c_header(tflite_path, header_path, array_name="model_tflite")

    print("Saved outputs in", OUT_DIR)
    print("Done")


if __name__ == "__main__":
    main()
