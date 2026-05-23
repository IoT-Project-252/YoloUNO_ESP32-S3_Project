# MQTT Extension - Cách chạy nhanh (ngắn gọn)

## 0) Mục tiêu
- Board A (YoloUNO): PIR phát hiện chuyển động -> publish MQTT
- Board B (ESP32-S3 DevKitC-1): nhận MQTT -> bật buzzer
- Broker: Mosquitto chạy local trên máy tính

---

## 1) Chạy Mosquitto broker
Mở **PowerShell #1**:

```powershell
& "C:\Program Files\mosquitto\mosquitto.exe" -c "C:\Program Files\mosquitto\mosquitto.conf" -v
```

Thấy dòng `opening ipv4 listen socket on port 1883` là OK.
Giữ cửa sổ này mở.

---

## 2) (Tùy chọn) Mở cửa sổ xem message MQTT
Mở **PowerShell #2**:

```powershell
& "C:\Program Files\mosquitto\mosquitto_sub.exe" -h localhost -t "iot/group1/#" -v
```

---

## 3) Lấy IP máy chạy broker
Mở **PowerShell #3**:

```powershell
ipconfig
```

Lấy IPv4 của Wi-Fi, ví dụ `192.168.1.5`.

---

## 4) Sửa MQTT_HOST trong code
- File A: `Src/include/pir_mqtt.h`
- File B: `ESP32-S3-DevKit-C1_Src/include/config.h`

Đặt cùng một IP:

```cpp
#define MQTT_HOST "192.168.1.5"
#define MQTT_PORT 1883
```

---

## 5) Nạp firmware cho Board A (YoloUNO)
Mở terminal tại thư mục project gốc:

```powershell
cd D:\Learning_HCMUT\HK252\IOT\YoloUNO_ESP32-S3_Project\Src
pio run -t upload
pio device monitor -b 115200
```

Nếu chưa có `upload_port`, thêm vào `Src/platformio.ini` đúng COM của Board A.

---

## 6) Kết nối WiFi cho Board A qua web
- Dùng điện thoại/laptop nối WiFi AP của board A (ví dụ `ESP32 LOCAL`)
- Mở `http://192.168.4.1`
- Vào `Settings` nhập SSID/password WiFi nhà/lab

Khi thành công, serial sẽ báo STA connected + MQTT connected.

---

## 7) Nạp firmware cho Board B (DevKitC-1)
Mở terminal khác:

```powershell
cd D:\Learning_HCMUT\HK252\IOT\YoloUNO_ESP32-S3_Project\ESP32-S3-DevKit-C1_Src
pio run -t upload
pio device monitor -b 115200
```

Nếu chưa có `upload_port`, thêm vào `ESP32-S3-DevKit-C1_Src/platformio.ini` đúng COM của Board B.

---

## 8) Test
- Vẫy tay trước PIR ở Board A
- Kết quả mong đợi:
  - Board A log: publish `motion_detected`
  - Board B log: nhận message và bật buzzer
  - Cửa sổ `mosquitto_sub` (nếu mở) thấy message qua lại

---

## 9) Lỗi hay gặp
1. Không connect MQTT:
- Sai `MQTT_HOST` (phải là IP máy tính, không phải `localhost` trên ESP32)
- 2 board không cùng WiFi LAN

2. Không upload được:
- Sai COM port (`pio device list` để kiểm tra)

3. Board A chưa publish:
- Chưa cấu hình WiFi STA ở trang `Settings`

4. Board B không kêu buzzer:
- Sai dây/mạch transistor, chưa nối GND chung với nguồn 5V
