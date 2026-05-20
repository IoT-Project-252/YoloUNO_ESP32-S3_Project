# Hướng Dẫn Chạy Task Mở Rộng: Giao Tiếp MQTT Giữa 2 Thiết Bị

## Mục lục

- [1. Tổng quan](#1-tổng-quan)
- [2. Danh sách phần cứng](#2-danh-sách-phần-cứng)
- [3. Sơ đồ đấu dây](#3-sơ-đồ-đấu-dây)
- [4. Cài đặt Mosquitto MQTT Broker](#4-cài-đặt-mosquitto-mqtt-broker)
- [5. Cấu hình code trước khi build](#5-cấu-hình-code-trước-khi-build)
- [6. Build và nạp code](#6-build-và-nạp-code)
- [7. Quy trình chạy từng bước](#7-quy-trình-chạy-từng-bước)
- [8. Kiểm tra và debug](#8-kiểm-tra-và-debug)
- [9. Xử lý lỗi thường gặp](#9-xử-lý-lỗi-thường-gặp)

---

## 1. Tổng quan

Task mở rộng này triển khai hệ thống cảnh báo chuyển động qua giao thức **MQTT** trong mạng LAN:

```
┌───────────────────┐         ┌──────────────┐         ┌───────────────────┐
│   Device A        │         │  Mosquitto   │         │   Device B        │
│   YoloUNO         │──MQTT──▶│  Broker      │──MQTT──▶│   DevKitC-1       │
│   (Cảm biến PIR)  │         │  (Máy tính)  │         │   (Buzzer 5V)     │
└───────────────────┘         └──────────────┘         └───────────────────┘
```

**Luồng hoạt động:**
1. PIR trên Device A phát hiện chuyển động → publish MQTT event
2. Mosquitto broker chuyển tiếp message
3. Device B nhận event → kích buzzer kêu trong 3 giây
4. Device B gửi ACK ngược lại cho Device A

---

## 2. Danh sách phần cứng

| STT | Linh kiện | Số lượng | Ghi chú |
|-----|-----------|----------|---------|
| 1 | ESP32-S3 YoloUNO | 1 | Device A (thiết bị chính) |
| 2 | ESP32-S3 DevKitC-1 | 1 | Device B (thiết bị phụ) |
| 3 | Cảm biến PIR (HC-SR501 hoặc AM312) | 1 | Phát hiện chuyển động |
| 4 | Buzzer 5V (active buzzer) | 1 | Còi báo động |
| 5 | Transistor NPN (2N2222 hoặc S8050) | 1 | Driver cho buzzer |
| 6 | Điện trở 1kΩ | 1 | Bảo vệ transistor |
| 7 | Diode 1N4007 (tuỳ chọn) | 1 | Flyback protection |
| 8 | Nguồn 5V ngoài | 1 | Cấp cho buzzer |
| 9 | Dây nối, breadboard | - | Đấu mạch |
| 10 | Máy tính Windows | 1 | Chạy Mosquitto broker |

---

## 3. Sơ đồ đấu dây

### 3.1. Device A — Cảm biến PIR → YoloUNO

```
Module PIR (HC-SR501 / AM312)
┌─────────────┐
│  VCC  ──────┼──▶ 3.3V trên YoloUNO (hoặc 5V tuỳ module)
│  OUT  ──────┼──▶ GPIO5 trên YoloUNO
│  GND  ──────┼──▶ GND trên YoloUNO
└─────────────┘
```

> **Lưu ý với HC-SR501:** Module có 2 núm vặn:
> - **Sensitivity (độ nhạy):** Vặn sang phải = nhạy hơn
> - **Time delay (thời gian giữ):** Vặn hết sang trái = khoảng 3 giây (ngắn nhất)

### 3.2. Device B — Buzzer 5V → DevKitC-1

**⚠️ KHÔNG cắm buzzer 5V trực tiếp vào chân GPIO của ESP32!**

Phải dùng mạch transistor NPN làm driver:

```
                           +5V (nguồn ngoài)
                              │
                         ┌────┴────┐
                         │ BUZZER  │
                         │  (+)    │
                         │  (-)    │
                         └────┬────┘
                              │
              ┌── (diode 1N4007, tuỳ chọn) ──┐
              │   cathode ↑       anode ↑     │
              │       nối +5V    nối collector │
              │                                │
                       ┌─────┐
                       │ NPN │  (2N2222 / S8050)
   GPIO4 ──[1kΩ]────▶ │  B  │
                       │  C ─┼──▶ Buzzer (-)
                       │  E ─┼──▶ GND (chung)
                       └─────┘

   ⚡ GND của ESP32 và GND của nguồn 5V PHẢI nối chung!
```

---

## 4. Cài đặt Mosquitto MQTT Broker

### Bước 4.1: Tải và cài đặt Mosquitto

1. Truy cập: https://mosquitto.org/download/
2. Tải bản **Windows 64-bit** (hoặc 32-bit tuỳ máy)
3. Chạy file cài đặt, nhấn **Next** cho đến khi xong
4. Mosquitto sẽ được cài vào `C:\Program Files\mosquitto\`

### Bước 4.2: Chỉnh file cấu hình

Mở **PowerShell** hoặc **Windows Terminal** dưới quyền **Administrator**:

```
Cách mở: Nhấn phím Win → gõ "PowerShell" → chuột phải → Run as administrator
```

Trong PowerShell Admin, gõ lệnh sau để mở file config bằng Notepad:

```powershell
notepad "C:\Program Files\mosquitto\mosquitto.conf"
```

Kéo xuống **cuối file**, thêm đúng 2 dòng sau:

```
listener 1883
allow_anonymous true
```

Giải thích:
- `listener 1883` → Cho phép các thiết bị trong LAN kết nối vào port 1883
- `allow_anonymous true` → Không yêu cầu username/password

Nhấn **Ctrl + S** để lưu, đóng Notepad.

### Bước 4.3: Xem IP máy tính

Mở một cửa sổ PowerShell (không cần Admin), gõ:

```powershell
ipconfig
```

Tìm phần **Wireless LAN adapter Wi-Fi** (nếu dùng WiFi), dòng:

```
IPv4 Address. . . . . . . . . . : 192.168.1.XX
```

**Ghi nhớ IP này** — ví dụ `192.168.1.5`. Đây là giá trị bạn sẽ điền vào `MQTT_HOST` trong code.

### Bước 4.4: Khởi động Mosquitto broker

Mở **PowerShell mới** (cửa sổ này phải giữ mở suốt quá trình test):

```powershell
& "C:\Program Files\mosquitto\mosquitto.exe" -c "C:\Program Files\mosquitto\mosquitto.conf" -v
```

Nếu thành công, bạn sẽ thấy:

```
1779205445: mosquitto version 2.x.x starting
1779205445: Config loaded from C:\Program Files\mosquitto\mosquitto.conf.
1779205445: Opening ipv4 listen socket on port 1883.
1779205445: mosquitto version 2.x.x running
```

**⚠️ Giữ nguyên cửa sổ PowerShell này, KHÔNG đóng!** Đóng = broker tắt.

### Bước 4.5 (tuỳ chọn): Mở terminal theo dõi message

Mở **thêm một cửa sổ PowerShell mới nữa**, gõ:

```powershell
& "C:\Program Files\mosquitto\mosquitto_sub.exe" -h localhost -t "iot/group1/#" -v
```

Cửa sổ này sẽ hiển thị **tất cả message MQTT** mà 2 ESP32 gửi qua lại — rất tiện để debug.

**Tóm lại bạn cần mở tối thiểu:**

| Cửa sổ | Lệnh | Vai trò |
|--------|-------|---------|
| PowerShell #1 | `mosquitto.exe -c ... -v` | Broker (bắt buộc) |
| PowerShell #2 | `mosquitto_sub.exe -h localhost -t ...` | Theo dõi (tuỳ chọn, nên mở) |

---

## 5. Cấu hình code trước khi build

### 5.1. Device A — File `Src/include/pir_mqtt.h`

Mở file và sửa các dòng sau cho đúng với hệ thống của bạn:

```cpp
#define MQTT_HOST       "192.168.1.5"    // ← Thay bằng IP máy tính (bước 4.3)
#define MQTT_PORT       1883             // Giữ nguyên
#define PIR_PIN         5                // ← GPIO nối PIR, đổi nếu cần
#define PIR_COOLDOWN_MS 3000             // Thời gian chờ giữa 2 lần gửi (ms)
```

### 5.2. Device B — File `ESP32-S3-DevKit-C1_Src/include/config.h`

```cpp
#define WIFI_SSID       "TenWiFiCuaBan"      // ← WiFi nhà/phòng lab
#define WIFI_PASS       "MatKhauWiFi"        // ← Mật khẩu WiFi
#define MQTT_HOST       "192.168.1.5"        // ← CÙNG IP máy tính như Device A
#define MQTT_PORT       1883
#define BUZZER_PIN      4                    // ← GPIO điều khiển transistor
```

> **Quan trọng:**
> - `MQTT_HOST` ở cả 2 device phải **giống nhau** và là IP **máy tính**, không phải IP ESP32.
> - `WIFI_SSID`/`WIFI_PASS` ở Device B là WiFi mà DevKitC-1 sẽ tự kết nối.
> - Device A không cần WiFi hardcode vì nó kết nối WiFi qua giao diện web.

---

## 6. Build và nạp code

### 6.1. Device A (YoloUNO)

1. Mở thư mục `Src/` bằng VS Code + PlatformIO
2. Cắm board YoloUNO qua USB
3. Nhấn nút **Build** (✓) ở thanh dưới VS Code
4. Nhấn nút **Upload** (→) để nạp code
5. Nhấn nút **Monitor** (🔌) để xem Serial log

### 6.2. Device B (DevKitC-1)

1. Mở thư mục `ESP32-S3-DevKit-C1_Src/` bằng VS Code + PlatformIO
2. Cắm board DevKitC-1 qua USB
3. Build → Upload → Monitor
4. **Sau khi upload xong**, nhấn nút **RST** (Reset) trên board nếu Serial Monitor không hiện gì

> **Mẹo:** Nếu bạn cần mở 2 project VS Code cùng lúc, có thể mở 2 cửa sổ VS Code riêng biệt.

---

## 7. Quy trình chạy từng bước

### Bước 1: Khởi động broker

Mở PowerShell, chạy lệnh ở mục 4.4.

### Bước 2: Bật Device A (YoloUNO)

Cắm USB và mở Serial Monitor. Bạn sẽ thấy:

```
=== ESP32-S3 IoT Project Boot ===
[PIR-MQTT] PIR sensor configured on GPIO5
[WebServer] AP started.
[WebServer] AP IP: 192.168.4.1
[PIR-MQTT] Waiting for WiFi STA connection...
[Main] All tasks created.
```

### Bước 3: Kết nối WiFi cho Device A

1. Dùng điện thoại/laptop kết nối WiFi **"ESP32 LOCAL"** (mật khẩu: `12345678`)
2. Mở trình duyệt, truy cập `http://192.168.4.1`
3. Vào trang **Settings**, nhập SSID và mật khẩu WiFi nhà/lab → nhấn **Connect**
4. Serial Monitor sẽ hiện:

```
[WiFi] STA connected! IP: 192.168.1.7
[PIR-MQTT] WiFi STA is connected. Starting MQTT...
[PIR-MQTT] Connecting to MQTT broker 192.168.1.5:1883 ...
[PIR-MQTT] MQTT connected!
[PIR-MQTT] Subscribed to: iot/group1/buzzer/state
```

### Bước 4: Bật Device B (DevKitC-1)

Cắm USB và mở Serial Monitor. Bạn sẽ thấy:

```
=== Device B: Buzzer Alarm Receiver ===
[Buzzer] Configured on GPIO4 (drives transistor)
[LED] NeoPixel blink task started on GPIO48
[WiFi] Connecting to TenWiFiCuaBan..........
[WiFi] Connected! IP: 192.168.1.8
[MQTT] Connecting to broker 192.168.1.5:1883 ...
[MQTT] Connected to broker!
[MQTT] Subscribed to: iot/group1/pir/events
[Main] All tasks created.
```

LED RGB trên DevKitC-1 sẽ nháy xanh dương 1 giây/lần.

### Bước 5: Test chuyển động

Vẫy tay trước cảm biến PIR trên Device A.

**Serial Device A:**
```
[PIR-MQTT] >>> Motion detected! Publishing event...
[PIR-MQTT] Published: {"device_id":"esp32s3-pir-01","event":"motion_detected","motion":true,"ts_ms":12345}
```

**Serial Device B:**
```
[MQTT] Received on [iot/group1/pir/events]: {"device_id":"esp32s3-pir-01",...}
[Buzzer] >>> Alarm triggered!
[Buzzer] ON for 3000 ms
[Buzzer] OFF
```

**Cửa sổ mosquitto_sub (nếu có):**
```
iot/group1/pir/events {"device_id":"esp32s3-pir-01","event":"motion_detected","motion":true,"ts_ms":12345}
iot/group1/buzzer/state {"device_id":"esp32s3-buzzer-01","alarm":"on","duration_ms":3000,"ts_ms":12350}
iot/group1/buzzer/state {"device_id":"esp32s3-buzzer-01","alarm":"off","duration_ms":3000,"ts_ms":15355}
```

---

## 8. Kiểm tra và debug

### Test 1: Broker hoạt động?

Trong PowerShell, gõ 2 lệnh ở 2 cửa sổ khác nhau:

**Cửa sổ 1 — subscribe:**
```powershell
& "C:\Program Files\mosquitto\mosquitto_sub.exe" -h localhost -t "test/#" -v
```

**Cửa sổ 2 — publish:**
```powershell
& "C:\Program Files\mosquitto\mosquitto_pub.exe" -h localhost -t "test/hello" -m "xin chao"
```

Nếu cửa sổ 1 hiện `test/hello xin chao` → broker OK.

### Test 2: ESP32 kết nối MQTT?

Xem Serial Monitor có dòng `[PIR-MQTT] MQTT connected!` hoặc `[MQTT] Connected to broker!`

### Test 3: PIR phát hiện chuyển động?

Xem Serial Monitor Device A có dòng `>>> Motion detected!` khi vẫy tay.

### Test 4: Mất mạng → tự kết nối lại?

1. Tắt Mosquitto (đóng cửa sổ PowerShell broker)
2. Cả 2 device sẽ log lỗi kết nối
3. Bật lại Mosquitto
4. Cả 2 device tự reconnect trong vòng ~5 giây

---

## 9. Xử lý lỗi thường gặp

| Lỗi | Nguyên nhân | Cách sửa |
|-----|-------------|----------|
| `MQTT connect failed, rc=-2` | Không kết nối được broker | Kiểm tra IP broker (`MQTT_HOST`), đảm bảo cùng mạng LAN |
| `Connection reset by peer` | ESP32 đang kết nối MQTT đến chính nó | `MQTT_HOST` phải là IP **máy tính**, không phải IP ESP32 |
| Device A không bắt đầu MQTT | Chưa kết nối WiFi STA | Vào web `192.168.4.1` → Settings → nhập WiFi |
| Device B kẹt ở "waiting for download" | Chưa reset board | Nhấn nút **RST** trên DevKitC-1 |
| Buzzer không kêu | Đấu dây sai hoặc thiếu transistor | Kiểm tra lại sơ đồ mục 3.2, thử test GPIO bằng LED trước |
| PIR không nhạy | Cooldown quá lớn hoặc PIR chưa warm up | Giảm `PIR_COOLDOWN_MS`, chờ PIR 30-60 giây sau khi cấp điện |
| `mosquitto.exe` báo lỗi port | Port 1883 bị chiếm | Tắt service Mosquitto đang chạy nền: `Stop-Service mosquitto` (PowerShell Admin) |
| Firewall chặn kết nối | Windows Firewall chặn port 1883 | Cho phép `mosquitto.exe` trong Windows Firewall |

### Cách mở port 1883 trên Windows Firewall (nếu bị chặn):

```powershell
# Chạy trong PowerShell Admin:
New-NetFirewallRule -DisplayName "Mosquitto MQTT" -Direction Inbound -Protocol TCP -LocalPort 1883 -Action Allow
```

---

## Cấu trúc file liên quan

```
YoloUNO_ESP32-S3_Project/
├── Src/                            ← Device A (YoloUNO)
│   ├── include/
│   │   ├── pir_mqtt.h              ← [MỚI] Config MQTT + PIR
│   │   ├── global.h
│   │   ├── actuators.h
│   │   └── ...
│   ├── src/
│   │   ├── pir_mqtt.cpp            ← [MỚI] PIR polling + MQTT publish
│   │   ├── main.cpp                ← [SỬA] Thêm pirMqttTask
│   │   └── ...
│   └── platformio.ini
│
├── ESP32-S3-DevKit-C1_Src/         ← Device B (DevKitC-1)
│   ├── include/
│   │   └── config.h                ← [MỚI] WiFi, MQTT, buzzer, LED config
│   ├── src/
│   │   └── main.cpp                ← [MỚI] MQTT subscriber + buzzer + LED blink
│   └── platformio.ini              ← [SỬA] Board + lib_deps
│
└── Docs/
    └── MQTT_Extension_README.md    ← File này
```
