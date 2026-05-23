#ifndef CONFIG_H
#define CONFIG_H

// ═══════════════════════════════════════════════════════════════════════════
// Device B Configuration — ESP32-S3 DevKitC-1 (Buzzer Alarm Receiver)
// ═══════════════════════════════════════════════════════════════════════════

// ── WiFi Configuration ──────────────────────────────────────────────────────
// The DevKitC-1 connects to an existing WiFi network (STA mode only).
// It must be on the same LAN as the Mosquitto broker and Device A.
#define WIFI_SSID       "BuiThiNgocKieu"
#define WIFI_PASS       "15051971"

// ── MQTT Broker Configuration ───────────────────────────────────────────────
#define MQTT_HOST       "192.168.1.9"   // Mosquitto broker IP in LAN
#define MQTT_PORT       1883
#define MQTT_USER       ""               // Leave empty if no auth
#define MQTT_PASS       ""               // Leave empty if no auth

// ── MQTT Topics ─────────────────────────────────────────────────────────────
#define MQTT_TOPIC_PIR_EVENT    "iot/group1/pir/events"
#define MQTT_TOPIC_BUZZER_STATE "iot/group1/buzzer/state"

// ── MQTT Client ID ──────────────────────────────────────────────────────────
#define MQTT_CLIENT_ID_B        "esp32s3-buzzer-01"

// ── Buzzer Configuration ────────────────────────────────────────────────────
// IMPORTANT: The buzzer is 5V. Do NOT connect it directly to the ESP32 GPIO.
// You MUST use a transistor/MOSFET driver circuit:
//
//   Wiring Diagram:
//   ───────────────
//
//   ESP32 GPIO4 ──[1kΩ resistor]──> NPN transistor Base (e.g. 2N2222)
//                                    |
//                              Emitter ──> GND (common ground)
//                                    |
//                            Collector ──> Buzzer (-)
//                                          Buzzer (+) ──> +5V supply
//
//   If using an active buzzer with inductive coil, add a 1N4007 flyback
//   diode across the buzzer terminals (cathode to +5V, anode to collector)
//   to protect the transistor from back-EMF spikes.
//
//   GND of ESP32 and GND of 5V supply MUST be connected together.
//
#define BUZZER_PIN              4        // GPIO4 — drives transistor base
#define ALARM_DURATION_MS       1000     // Buzzer ON time per event (3 seconds)

// ── Behavior on overlapping events ──────────────────────────────────────────
// Strategy: IGNORE new events while buzzer is currently active.
// This prevents retriggering and ensures predictable alarm duration.
// Alternative would be to extend/restart the timer, but ignore is simpler
// and avoids indefinite alarm if PIR keeps firing.

// ── Onboard RGB LED (NeoPixel) Configuration ────────────────────────────────
// ESP32-S3 DevKitC-1 has an onboard WS2812 (NeoPixel) RGB LED on GPIO48.
#define NEOPIXEL_LED_PIN        48
#define NEOPIXEL_LED_COUNT      1

#endif
