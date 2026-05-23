#include "task_coreiot.h"

// Initialize the WiFi and MQTT clients
static WiFiClient   wifiClient;
static PubSubClient mqttClient(wifiClient);

// ----- RPC Callbacks (Receiving commands from Server) -----

static void callback(char* topic, byte* payload, unsigned int length)
{
    // Convert payload to string
    String msg;
    msg.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }

    // Parse JSON payload
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        return; // Ignore invalid JSON
    }

    const char* method = doc["method"];
    bool params = doc["params"]; // true for ON (Override), false for OFF (Auto)
    bool validMethod = false;

    // Route the command based on the Override flags
    if (String(method) == "setBuiltInLED") 
    {
        validMethod = true;
        // DATA MUTEX: Safely update the Override flag for Task 1 LED
        if (xSemaphoreTake(projectSharedData->mutex, portMAX_DELAY) == pdTRUE) {
            projectSharedData->forceBuiltInLED = params;
            xSemaphoreGive(projectSharedData->mutex);
        }
        if (xSemaphoreTake(projectSharedData->serialMutex, portMAX_DELAY) == pdTRUE) {
            Serial.printf("[CoreIOT] RPC LED Override: %d\n", params);
            Serial.flush();
            xSemaphoreGive(projectSharedData->serialMutex);
        }
    } 
    else if (String(method) == "setNeoPixelLED") 
    {
        validMethod = true;
        // DATA MUTEX: Safely update the Override flag for Task 2 NeoPixel LED
        if (xSemaphoreTake(projectSharedData->mutex, portMAX_DELAY) == pdTRUE) {
            projectSharedData->forceNeoPixelLED = params;
            xSemaphoreGive(projectSharedData->mutex);
        }
        if (xSemaphoreTake(projectSharedData->serialMutex, portMAX_DELAY) == pdTRUE) {
            Serial.printf("[CoreIOT] RPC NeoPixel LED Override: %d\n", params);
            Serial.flush();
            xSemaphoreGive(projectSharedData->serialMutex);
        }
    }

    // Acknowledge the RPC to prevent timeout error on the Dashboard
    if (validMethod) {
        String topicStr = String(topic);
        int reqIdx = topicStr.lastIndexOf('/') + 1;
        String reqId = topicStr.substring(reqIdx);
        String responseTopic = "v1/devices/me/rpc/response/" + reqId;
        mqttClient.publish(responseTopic.c_str(), "{}");
    }
}

// ------- Main CoreIOT Task -------

void CoreIoT_Task(void *pvParameters)
{
    // Retrieve the shared data pointer
    SharedData* sharedData = (SharedData*) pvParameters;

    // Configure MQTT broker
    mqttClient.setServer(CORE_IOT_SERVER, CORE_IOT_PORT);
    mqttClient.setCallback(callback);
    
    float currentTemp = 0.0f;
    float currentHum = 0.0f;
    uint32_t lastSendTime = 0;          // Non-blocking timer variable

    // Main infinite loop for MQTT communication
    while (1) 
    {
        // Only attempt server operations if the WiFi connection is active
        if (sharedData->staConnected) 
        {
            // Connect to CoreIOT if disconnected
            if (!mqttClient.connected()) 
            {
                if (xSemaphoreTake(sharedData->serialMutex, portMAX_DELAY) == pdTRUE) 
                {
                    Serial.println("[CoreIOT] Connecting to CoreIOT Server...");
                    xSemaphoreGive(sharedData->serialMutex);
                }

                // Initiate connection with server URL and device token
                if (mqttClient.connect("YoloUNO_Node", CORE_IOT_TOKEN, "")) 
                {
                    // Subscribe to incoming RPC commands
                    mqttClient.subscribe("v1/devices/me/rpc/request/+");
                    
                    if (xSemaphoreTake(sharedData->serialMutex, portMAX_DELAY) == pdTRUE) 
                    {
                        Serial.println("[CoreIOT] Connected successfully! RPC Subscribed.");
                        Serial.flush();
                        xSemaphoreGive(sharedData->serialMutex);
                    }
                }
            }

            // If the connection is established, process the event loop and publish data
            if (mqttClient.connected()) 
            {
                // Maintain MQTT connection and process any incoming messages
                mqttClient.loop(); 

                // Check the timer: Only send Telemetry after 10 seconds have elapsed
                if (millis() - lastSendTime > 5000) {
                    lastSendTime = millis();

                    // DATA MUTEX: Securely retrieve sensor readings from shared memory
                    if (xSemaphoreTake(sharedData->mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
                    {
                        currentTemp = sharedData->temperature;
                        currentHum  = sharedData->humidity;
                        xSemaphoreGive(sharedData->mutex);
                    }

                    // Package it into a standard JSON string to ensure the server recognizes it.
                    char payload[128];
                    snprintf(payload, sizeof(payload), "{\"temperature\":%.2f, \"humidity\":%.2f}", currentTemp, currentHum);

                    // Publish to CoreIOT standard telemetry topic
                    if (mqttClient.publish("v1/devices/me/telemetry", payload)) 
                    {
                        if (xSemaphoreTake(sharedData->serialMutex, portMAX_DELAY) == pdTRUE) {
                            Serial.printf("[CoreIOT] Telemetry published: %s\n", payload);
                            Serial.flush();
                            xSemaphoreGive(sharedData->serialMutex);
                        }
                    } 
                    else 
                    {
                        if (xSemaphoreTake(sharedData->serialMutex, portMAX_DELAY) == pdTRUE) {
                            Serial.println("[CoreIOT] Telemetry publish FAILED!");
                            Serial.flush();
                            xSemaphoreGive(sharedData->serialMutex);
                        }
                    }
                }
            }
        }
        
        // Wait before the next telemetry transmission cycle
        vTaskDelay(pdMS_TO_TICKS(500)); 
    }
}