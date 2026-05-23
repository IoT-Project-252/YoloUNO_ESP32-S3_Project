#ifndef PIR_MQTT_H
#define PIR_MQTT_H

#include "global.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ── MQTT Broker Configuration ───────────────────────────────────────────────
// Override these via build_flags in platformio.ini, e.g.:
//   -DMQTT_HOST="\"192.168.1.10\""
#ifndef MQTT_HOST
#define MQTT_HOST       "192.168.1.9"   // Mosquitto broker IP in LAN
#endif

#ifndef MQTT_PORT
#define MQTT_PORT       1883
#endif

#ifndef MQTT_USER
#define MQTT_USER       ""               // Leave empty if no auth
#endif

#ifndef MQTT_PASS
#define MQTT_PASS       ""               // Leave empty if no auth
#endif

// ── MQTT Topics ─────────────────────────────────────────────────────────────
#define MQTT_TOPIC_PIR_EVENT    "iot/group1/pir/events"
#define MQTT_TOPIC_BUZZER_STATE "iot/group1/buzzer/state"

// ── MQTT Client ID ──────────────────────────────────────────────────────────
#define MQTT_CLIENT_ID_A        "esp32s3-pir-01"

// ── PIR Sensor Configuration ────────────────────────────────────────────────
// PIR output is digital HIGH when motion is detected.
// Most PIR modules (HC-SR501, AM312) output HIGH on motion.
// Use INPUT_PULLDOWN to ensure clean LOW when idle.
#define PIR_PIN                 8        // D5 on YoloUNO
#define PIR_COOLDOWN_MS         2000     // Minimum 5s between publishes

// ── FreeRTOS Task ───────────────────────────────────────────────────────────
// This task handles:
//   1. Connecting to MQTT broker (requires WiFi STA to be up first)
//   2. Reading PIR sensor
//   3. Publishing motion events with cooldown
void pirMqttTask(void *pvParameters);

#endif
