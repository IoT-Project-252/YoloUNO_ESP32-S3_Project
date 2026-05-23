# YOLOUNO ESP32-S3 IoT Project

## 1) Giới thiệu

Đây là project Bài tập lớn môn Phát triển Ứng dụng IoT, triển khai trên **YoloUNO (ESP32-S3)** theo kiến trúc **FreeRTOS đa task**.  
Hệ thống hỗ trợ:

- Giám sát nhiệt độ/độ ẩm từ DHT20
- Cảnh báo bằng LED đơn + NeoPixel + LCD
- Dashboard web chạy ở chế độ AP/AP+STA để điều khiển LED RGB và quạt
- Đẩy telemetry lên CoreIOT qua MQTT
- TinyML inference trên ESP32-S3
- Mở rộng giao tiếp 2 thiết bị qua local MQTT broker (PIR -> buzzer)

---

## 2) Chức năng theo task

- **Task 1**: LED đơn nháy theo ngưỡng nhiệt độ (3 mức)
- **Task 2**: NeoPixel built-in đổi màu theo độ ẩm
- **Task 3**: LCD hiển thị trạng thái `NORMAL / WARNING / CRITICAL`
- **Task 4**: Web Server AP mode + trang `/settings` để nhập Wi-Fi STA
- **Task 5**: TinyML (TFLite Micro) phân loại trạng thái môi trường
- **Task 6**: CoreIOT MQTT telemetry + RPC override
- **Task mở rộng**: Node A (PIR) gửi MQTT, Node B (DevKitC-1) kích buzzer 5V

---

## 3) Cấu trúc thư mục

```text
YoloUNO_ESP32-S3_Project/
├─ Src/                         # Firmware chính cho YoloUNO (Device A)
│  ├─ src/                      # Các task FreeRTOS
│  ├─ include/                  # Header + cấu hình task
│  ├─ tinyML/                   # Script train/convert/evaluate model
│  └─ platformio.ini
├─ ESP32-S3-DevKit-C1_Src/      # Firmware cho Device B (buzzer receiver)
│  ├─ src/main.cpp
│  ├─ include/config.h
│  └─ platformio.ini
└─ Docs/                        # Đặc tả + báo cáo LaTeX + ảnh minh họa
```

---

## 4) Phần cứng sử dụng

- **Device A (YoloUNO ESP32-S3)**:
  - DHT20 (I2C)
  - LCD 1602 I2C
  - LED built-in
  - NeoPixel built-in
  - NeoPixel RGB rời (4 LED)
  - Quạt mini 5V (qua mạch driver PWM)
  - PIR (cho task mở rộng MQTT)

- **Device B (ESP32-S3 DevKitC-1)**:
  - Buzzer 5V (qua transistor/MOSFET driver)

---

## 5) Mapping chân GPIO (theo code hiện tại)

### Device A - YoloUNO (`Src/include/*.h`)

| Thành phần | GPIO |
|---|---|
| DHT20 SDA | 11 |
| DHT20 SCL | 12 |
| LED built-in (Task 1) | 48 |
| NeoPixel built-in (Task 2) | 45 |
| NeoPixel rời (Web actuator) | 6 |
| Quạt PWM (Web actuator) | 1 |
| PIR | 8 |

Lưu ý: Có **2 hệ LED RGB khác nhau**:
- NeoPixel built-in (Task 2, độ ẩm): `GPIO45`
- NeoPixel rời (dashboard web): `GPIO6`, `NEOPIXEL_COUNT = 4`

### Device B - DevKitC-1 (`ESP32-S3-DevKit-C1_Src/include/config.h`)

| Thành phần | GPIO |
|---|---|
| Buzzer driver control | 4 |
| NeoPixel onboard blink | 48 |

---

## 6) Yêu cầu phần mềm

- VS Code + PlatformIO extension
- Python 3.9+ (nếu muốn train lại TinyML)
- Mosquitto (nếu chạy task mở rộng local MQTT)

---

## 7) Cấu hình quan trọng trước khi chạy

### 7.1 AP mode của web server (Device A)

Trong `Src/platformio.ini`:

```ini
-DSSID_AP="\"ESP32 LOCAL\""
-DPASS_AP="\"12345678\""
```

### 7.2 CoreIOT token (Device A)

Trong `Src/include/task_coreiot.h`:

```cpp
#define CORE_IOT_SERVER "app.coreiot.io"
#define CORE_IOT_TOKEN  "YOUR_DEVICE_TOKEN"
#define CORE_IOT_PORT   1883
```

### 7.3 MQTT local broker (task mở rộng)

- Device A: `Src/include/pir_mqtt.h` -> `MQTT_HOST`
- Device B: `ESP32-S3-DevKit-C1_Src/include/config.h` -> `MQTT_HOST`

Hai file phải dùng **cùng IP máy chạy Mosquitto** trong LAN.

---

## 8) Chạy firmware chính (Device A - YoloUNO)

### 8.1 Nạp code

```powershell
cd Src
pio run -t upload
pio device monitor -b 115200
```

Nếu sai COM, thêm `upload_port = COMx` vào `Src/platformio.ini`.

### 8.2 Vào dashboard AP

1. Kết nối Wi-Fi AP của board A (mặc định `ESP32 LOCAL`)
2. Mở `http://192.168.4.1`
3. Dashboard hiển thị:
   - Nhiệt độ/độ ẩm
   - Điều khiển LED RGB rời
   - Điều khiển quạt

### 8.3 Cấu hình STA

1. Vào `http://192.168.4.1/settings`
2. Nhập SSID/password Wi-Fi thật
3. Kết nối thành công -> board chạy AP+STA, task cloud/MQTT hoạt động

---

## 9) API web server (Device A)

| Endpoint | Method | Chức năng |
|---|---|---|
| `/` | GET | Trang dashboard |
| `/settings` | GET | Trang cài Wi-Fi STA |
| `/api/state` | GET | Lấy trạng thái sensor + actuator + STA |
| `/api/led?on=&r=&g=&b=` | GET | Điều khiển LED RGB rời |
| `/api/fan?on=&speed=` | GET | Điều khiển quạt PWM |
| `/api/wifi` | POST (JSON) | Lưu SSID/password và thử connect STA |

Ví dụ body `/api/wifi`:

```json
{
  "ssid": "YourWiFi",
  "password": "YourPassword"
}
```

---

## 10) Chạy task mở rộng 2 thiết bị (PIR -> Buzzer qua Mosquitto)

### 10.1 Chạy broker trên máy tính

```powershell
& "C:\Program Files\mosquitto\mosquitto.exe" -c "C:\Program Files\mosquitto\mosquitto.conf" -v
```

Tùy chọn monitor:

```powershell
& "C:\Program Files\mosquitto\mosquitto_sub.exe" -h localhost -t "iot/group1/#" -v
```

### 10.2 Nạp Device A và Device B

Device A:

```powershell
cd Src
pio run -t upload
pio device monitor -b 115200
```

Device B:

```powershell
cd ESP32-S3-DevKit-C1_Src
pio run -t upload
pio device monitor -b 115200
```

### 10.3 MQTT topics

- `iot/group1/pir/events` (A -> B)
- `iot/group1/buzzer/state` (B -> A ACK)

### 10.4 Kết quả mong đợi

- PIR phát hiện chuyển động -> Device A publish `motion_detected`
- Device B nhận message -> bật buzzer trong `ALARM_DURATION_MS`
- Device B publish ACK trạng thái buzzer

---

## 11) TinyML (tùy chọn train lại model)

Thư mục: `Src/tinyML/`

Các script chính:

- `generate_dataset.py`
- `train_and_convert.py`
- `evaluate_tflite.py`
- `mock_scenarios.py`

Thư viện Python:

```powershell
cd Src/tinyML
pip install -r requirements.txt
```

Khi train xong, model header được dùng bởi firmware tại:
- `Src/tinyML/output/model_data.h`
- `Src/tinyML/output/scaler_params.json`

---

## 12) Lỗi thường gặp và cách xử lý

1. **Không upload được firmware**
- Kiểm tra COM bằng `pio device list`
- Đặt `upload_port` đúng cho từng board

2. **Không vào được dashboard**
- Đảm bảo board đã boot và AP đã phát
- Truy cập đúng `192.168.4.1`

3. **STA không connect**
- Sai SSID/password
- Router chặn thiết bị mới

4. **MQTT extension không chạy**
- `MQTT_HOST` sai (ESP32 phải dùng IP LAN của máy chạy broker, không dùng `localhost`)
- Device A và B không cùng mạng Wi-Fi

5. **Buzzer 5V không kêu hoặc ESP reset**
- Không được nối trực tiếp buzzer 5V vào GPIO
- Dùng transistor/MOSFET driver và nối mass chung

---

## 13) Ghi chú bảo mật

- Không commit public các thông tin nhạy cảm:
  - Wi-Fi SSID/password
  - CoreIOT token
  - MQTT user/password
- Nên chuyển các thông số này sang `build_flags` hoặc file cấu hình local.

---

## 14) Nhóm thực hiện

- Lê Hoàng Tân
- Đỗ Đăng Khoa
- Lê Thành Nhân

