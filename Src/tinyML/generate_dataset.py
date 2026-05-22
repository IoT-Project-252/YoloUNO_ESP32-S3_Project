"""Generate a synthetic sensor dataset: temperature, humidity, label

Labels:
0 = Normal
1 = Temperature anomaly
2 = Humidity anomaly

Writes sensor_data.csv (no header) into the same folder.
"""
import numpy as np
import os

OUT = "sensor_data.csv"

def gen_normal(n):
    # realistic normal: temp 22-32, humidity 45-75, small gaussian noise
    temps = np.random.normal(loc=27, scale=2.5, size=n)
    temps = np.clip(temps, 22, 32)
    hums = np.random.normal(loc=60, scale=7, size=n)
    hums = np.clip(hums, 45, 75)
    labels = np.zeros(n, dtype=int)
    return temps, hums, labels

def gen_temp_anomaly(n):
    # half very high, half very low
    n2 = n // 2
    high = np.random.normal(loc=45, scale=3.5, size=n2)  # heat spikes
    low = np.random.normal(loc=5, scale=2.0, size=n - n2)  # cold spikes
    temps = np.concatenate([high, low])
    temps = np.clip(temps, -10, 60)
    # humidity remains somewhat in normal-ish range but can vary
    hums = np.random.normal(loc=55, scale=10, size=n)
    hums = np.clip(hums, 20, 90)
    labels = np.ones(n, dtype=int)
    return temps, hums, labels

def gen_humi_anomaly(n):
    # half very high humidity, half very low
    n2 = n // 2
    high = np.random.normal(loc=95, scale=2.5, size=n2)
    low = np.random.normal(loc=10, scale=4.0, size=n - n2)
    hums = np.concatenate([high, low])
    hums = np.clip(hums, 0, 100)
    temps = np.random.normal(loc=27, scale=6.0, size=n)
    temps = np.clip(temps, -5, 50)
    labels = np.full(n, 2, dtype=int)
    return temps, hums, labels

def build_dataset(total_samples=1800, outpath=None, seed=42):
    if seed is not None:
        np.random.seed(seed)
    # Ensure roughly balanced classes
    per_class = total_samples // 3
    t1, h1, l1 = gen_normal(per_class)
    t2, h2, l2 = gen_temp_anomaly(per_class)
    t3, h3, l3 = gen_humi_anomaly(per_class)

    temps = np.concatenate([t1, t2, t3])
    hums = np.concatenate([h1, h2, h3])
    labels = np.concatenate([l1, l2, l3])

    # shuffle
    idx = np.arange(len(labels))
    np.random.shuffle(idx)
    temps = temps[idx]
    hums = hums[idx]
    labels = labels[idx]

    if outpath is None:
        outpath = os.path.join(os.path.dirname(__file__), OUT)

    # Save as space-separated with 5 decimals to match the requested view
    with open(outpath, "w") as f:
        for t, h, lbl in zip(temps, hums, labels):
            f.write(f"{t:8.5f} {h:8.5f} {lbl}\n")

    print(f"Wrote {len(labels)} samples to {outpath}")


if __name__ == "__main__":
    build_dataset(1800)
