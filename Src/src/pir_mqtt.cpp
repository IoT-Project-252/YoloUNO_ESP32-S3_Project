#include "pir_mqtt.h"

// ── Static instances (local to this translation unit) ───────────────────────
static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

// ── MQTT reconnect helper ───────────────────────────────────────────────────
// Attempts to connect to the MQTT broker. Returns true on success.
// Non-blocking: tries once per call, caller should retry with delay.
static bool mqttReconnect()
{
    if (mqttClient.connected()) return true;

    Serial.printf("[PIR-MQTT] Connecting to MQTT broker %s:%d ...\n", MQTT_HOST, MQTT_PORT);

    bool connected = false;
    if (strlen(MQTT_USER) > 0) {
        connected = mqttClient.connect(MQTT_CLIENT_ID_A, MQTT_USER, MQTT_PASS);
    } else {
        connected = mqttClient.connect(MQTT_CLIENT_ID_A);
    }

    if (connected) {
        Serial.println("[PIR-MQTT] MQTT connected!");
        // Subscribe to buzzer ACK topic (optional, for logging)
        mqttClient.subscribe(MQTT_TOPIC_BUZZER_STATE);
        Serial.printf("[PIR-MQTT] Subscribed to: %s\n", MQTT_TOPIC_BUZZER_STATE);
    } else {
        Serial.printf("[PIR-MQTT] MQTT connect failed, rc=%d. Will retry...\n",
                       mqttClient.state());
    }
    return connected;
}

// ── MQTT incoming message callback (for buzzer ACK) ─────────────────────────
static void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    // Print received ACK from Device B for debugging
    String msg;
    msg.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.printf("[PIR-MQTT] Received on [%s]: %s\n", topic, msg.c_str());
}

// ── Publish a motion event ──────────────────────────────────────────────────
static void publishMotionEvent(bool motionDetected)
{
    StaticJsonDocument<200> doc;
    doc["device_id"] = MQTT_CLIENT_ID_A;
    doc["event"]     = motionDetected ? "motion_detected" : "motion_cleared";
    doc["motion"]    = motionDetected;
    doc["ts_ms"]     = (unsigned long)millis();

    char buffer[200];
    size_t n = serializeJson(doc, buffer, sizeof(buffer));

    if (mqttClient.publish(MQTT_TOPIC_PIR_EVENT, buffer, n)) {
        Serial.printf("[PIR-MQTT] Published: %s\n", buffer);
    } else {
        Serial.println("[PIR-MQTT] Publish failed!");
    }
}

// ── FreeRTOS Task Implementation ────────────────────────────────────────────
//
// Flow:
//   1. Wait until WiFi STA is connected (checked via SharedData->staConnected).
//   2. Connect to MQTT broker.
//   3. Poll PIR pin every 200ms.
//   4. On rising edge (LOW->HIGH), publish event with cooldown protection.
//   5. Maintain MQTT connection (auto-reconnect).
//
void pirMqttTask(void *pvParameters)
{
    SharedData* sd = (SharedData*) pvParameters;

    // --- Configure PIR pin ---
    // Most PIR modules output HIGH on detection, LOW when idle.
    // INPUT_PULLDOWN ensures stable LOW when no signal from PIR module.
    pinMode(PIR_PIN, INPUT_PULLDOWN);
    Serial.printf("[PIR-MQTT] PIR sensor configured on GPIO%d\n", PIR_PIN);

    // --- Configure MQTT client ---
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(60);

    // --- Wait for WiFi STA connection ---
    Serial.println("[PIR-MQTT] Waiting for WiFi STA connection...");
    while (!sd->staConnected) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    Serial.println("[PIR-MQTT] WiFi STA is connected. Starting MQTT...");

    // --- State tracking ---
    bool     lastPirState    = false;    // Previous PIR reading
    uint32_t lastPublishTime = 0;        // millis() of last publish
    uint32_t lastReconnectAttempt = 0;   // millis() of last MQTT reconnect try

    // --- Main loop ---
    while (1) {
        // --- Check WiFi STA ---
        if (!sd->staConnected || WiFi.status() != WL_CONNECTED) {
            Serial.println("[PIR-MQTT] WiFi lost. Waiting for reconnection...");
            while (!sd->staConnected || WiFi.status() != WL_CONNECTED) {
                vTaskDelay(pdMS_TO_TICKS(2000));
            }
            Serial.println("[PIR-MQTT] WiFi restored.");
        }

        // --- Maintain MQTT connection ---
        if (!mqttClient.connected()) {
            uint32_t now = millis();
            // Retry every 5 seconds to avoid hammering the broker
            if (now - lastReconnectAttempt >= 5000) {
                lastReconnectAttempt = now;
                mqttReconnect();
            }
        }

        // --- Process MQTT (handles keepalive + incoming messages) ---
        mqttClient.loop();

        // --- Read PIR sensor ---
        bool currentPir = digitalRead(PIR_PIN) == HIGH;

        // Detect rising edge (no motion -> motion detected)
        if (currentPir && !lastPirState) {
            uint32_t now = millis();

            // Cooldown check: only publish if enough time has elapsed
            if (now - lastPublishTime >= PIR_COOLDOWN_MS) {
                Serial.println("[PIR-MQTT] >>> Motion detected! Publishing event...");
                publishMotionEvent(true);
                lastPublishTime = now;
            } else {
                Serial.println("[PIR-MQTT] Motion detected but cooldown active. Skipping.");
            }
        }

        lastPirState = currentPir;

        // Poll interval: 200ms is responsive enough for PIR
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}
