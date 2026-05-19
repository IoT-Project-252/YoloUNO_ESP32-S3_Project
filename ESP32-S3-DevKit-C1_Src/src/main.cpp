#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// ═══════════════════════════════════════════════════════════════════════════
// Device B — ESP32-S3 DevKitC-1
// Role: MQTT subscriber that activates a 5V buzzer when PIR motion is
//       detected by Device A.
//
// Architecture:
//   - FreeRTOS Task 1 (mqttTask): WiFi + MQTT connection management,
//     receives PIR events and sets a flag.
//   - FreeRTOS Task 2 (buzzerTask): Monitors the flag and drives the
//     buzzer GPIO for a fixed duration.
// ═══════════════════════════════════════════════════════════════════════════

// ── Global state (protected by mutex) ───────────────────────────────────────
static SemaphoreHandle_t alarmMutex;
static volatile bool     alarmActive   = false;   // Is buzzer currently ON?
static volatile bool     alarmTrigger  = false;   // New event received?

// ── WiFi + MQTT instances ───────────────────────────────────────────────────
static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

// ── WiFi connect/reconnect ──────────────────────────────────────────────────
static void wifiConnect()
{
    if (WiFi.status() == WL_CONNECTED) return;

    Serial.printf("[WiFi] Connecting to %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WiFi] Connection failed. Will retry...");
    }
}

// ── Publish buzzer ACK ──────────────────────────────────────────────────────
static void publishBuzzerAck(bool alarmOn)
{
    StaticJsonDocument<200> doc;
    doc["device_id"]    = MQTT_CLIENT_ID_B;
    doc["alarm"]        = alarmOn ? "on" : "off";
    doc["duration_ms"]  = ALARM_DURATION_MS;
    doc["ts_ms"]        = (unsigned long)millis();

    char buffer[200];
    size_t n = serializeJson(doc, buffer, sizeof(buffer));

    if (mqttClient.publish(MQTT_TOPIC_BUZZER_STATE, buffer, n)) {
        Serial.printf("[MQTT] ACK published: %s\n", buffer);
    } else {
        Serial.println("[MQTT] ACK publish failed!");
    }
}

// ── MQTT incoming message callback ──────────────────────────────────────────
static void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    // Parse incoming JSON
    String msg;
    msg.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.printf("[MQTT] Received on [%s]: %s\n", topic, msg.c_str());

    // Only process PIR event topic
    if (String(topic) != MQTT_TOPIC_PIR_EVENT) return;

    StaticJsonDocument<200> doc;
    DeserializationError err = deserializeJson(doc, msg);
    if (err) {
        Serial.printf("[MQTT] JSON parse error: %s\n", err.c_str());
        return;
    }

    bool motion = doc["motion"] | false;
    const char* event = doc["event"] | "";

    if (motion && strcmp(event, "motion_detected") == 0) {
        // Check if alarm is already active — if so, IGNORE (strategy: no overlap)
        if (xSemaphoreTake(alarmMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (alarmActive) {
                Serial.println("[Buzzer] Alarm already active. Ignoring new event.");
            } else {
                alarmTrigger = true;
                Serial.println("[Buzzer] >>> Alarm triggered!");
            }
            xSemaphoreGive(alarmMutex);
        }
    }
}

// ── MQTT reconnect helper ───────────────────────────────────────────────────
static bool mqttReconnect()
{
    if (mqttClient.connected()) return true;

    Serial.printf("[MQTT] Connecting to broker %s:%d ...\n", MQTT_HOST, MQTT_PORT);

    bool connected = false;
    if (strlen(MQTT_USER) > 0) {
        connected = mqttClient.connect(MQTT_CLIENT_ID_B, MQTT_USER, MQTT_PASS);
    } else {
        connected = mqttClient.connect(MQTT_CLIENT_ID_B);
    }

    if (connected) {
        Serial.println("[MQTT] Connected to broker!");
        mqttClient.subscribe(MQTT_TOPIC_PIR_EVENT);
        Serial.printf("[MQTT] Subscribed to: %s\n", MQTT_TOPIC_PIR_EVENT);
    } else {
        Serial.printf("[MQTT] Connect failed, rc=%d. Will retry...\n", mqttClient.state());
    }
    return connected;
}

// ═══════════════════════════════════════════════════════════════════════════
// FreeRTOS Task: MQTT Connection Manager
// Handles WiFi reconnection, MQTT reconnection, and message processing.
// ═══════════════════════════════════════════════════════════════════════════
static void mqttTask(void *pvParameters)
{
    // --- Initial WiFi connection ---
    wifiConnect();

    // --- Configure MQTT ---
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(60);

    uint32_t lastReconnectAttempt = 0;

    while (1) {
        // --- WiFi health check ---
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFi] Lost connection. Reconnecting...");
            wifiConnect();
        }

        // --- MQTT health check ---
        if (!mqttClient.connected()) {
            uint32_t now = millis();
            if (now - lastReconnectAttempt >= 5000) {
                lastReconnectAttempt = now;
                mqttReconnect();
            }
        }

        // --- Process incoming MQTT messages ---
        mqttClient.loop();

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// FreeRTOS Task: Buzzer Driver
// Monitors the alarm trigger flag and drives the buzzer GPIO.
// Uses non-blocking timing (no delay() blocking other tasks).
// ═══════════════════════════════════════════════════════════════════════════
static void buzzerTask(void *pvParameters)
{
    // Configure buzzer pin as output (drives transistor base)
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);   // Ensure buzzer is OFF at startup
    Serial.printf("[Buzzer] Configured on GPIO%d (drives transistor)\n", BUZZER_PIN);

    while (1) {
        bool shouldActivate = false;

        // Check trigger flag
        if (xSemaphoreTake(alarmMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (alarmTrigger) {
                alarmTrigger = false;
                alarmActive  = true;
                shouldActivate = true;
            }
            xSemaphoreGive(alarmMutex);
        }

        if (shouldActivate) {
            // --- Turn buzzer ON ---
            digitalWrite(BUZZER_PIN, LOW);
            Serial.printf("[Buzzer] ON for %d ms\n", ALARM_DURATION_MS);

            // Publish ACK to MQTT
            publishBuzzerAck(true);

            // Wait for alarm duration (non-blocking to other tasks)
            vTaskDelay(pdMS_TO_TICKS(ALARM_DURATION_MS));

            // --- Turn buzzer OFF ---
            digitalWrite(BUZZER_PIN, HIGH);
            Serial.println("[Buzzer] OFF");

            // Publish OFF ACK
            publishBuzzerAck(false);

            // Clear active flag
            if (xSemaphoreTake(alarmMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
                alarmActive = false;
                xSemaphoreGive(alarmMutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// FreeRTOS Task: RGB LED Blink (1 second interval)
// Blinks the onboard NeoPixel RGB LED on GPIO48 to show board is alive.
// ═══════════════════════════════════════════════════════════════════════════
static Adafruit_NeoPixel onboardLed(NEOPIXEL_LED_COUNT, NEOPIXEL_LED_PIN, NEO_GRB + NEO_KHZ800);

static void blinkLedTask(void *pvParameters)
{
    onboardLed.begin();
    onboardLed.setBrightness(50);  // Not too bright
    onboardLed.show();
    Serial.printf("[LED] NeoPixel blink task started on GPIO%d\n", NEOPIXEL_LED_PIN);

    while (1) {
        // ON — blue color
        onboardLed.setPixelColor(0, onboardLed.Color(0, 0, 255));
        onboardLed.show();
        vTaskDelay(pdMS_TO_TICKS(1000));

        // OFF
        onboardLed.setPixelColor(0, 0);
        onboardLed.show();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// ═══════════════════════════════════════════════════════════════════════════
// Arduino setup() and loop()
// ═══════════════════════════════════════════════════════════════════════════
void setup()
{
    Serial.begin(115200);
    delay(500);
    Serial.println("\n=== Device B: Buzzer Alarm Receiver ===");

    // Create mutex for alarm state
    alarmMutex = xSemaphoreCreateMutex();

    // Create FreeRTOS tasks
    xTaskCreate(mqttTask,    "MqttTask",   6144, NULL, 1, NULL);
    xTaskCreate(buzzerTask,  "BuzzerTask", 4096, NULL, 1, NULL);
    xTaskCreate(blinkLedTask,"BlinkLED",   4096, NULL, 1, NULL);

    Serial.println("[Main] All tasks created.");
}

void loop()
{
    // FreeRTOS handles everything; delete Arduino loop task
    vTaskDelete(NULL);
}