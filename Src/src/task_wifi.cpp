#include "task_wifi.h"

bool WiFi_reconnect(SharedData* sharedData) {
    // Check if the SSID has been configured via the Web Server
    // If the string length is 0, the user hasn't set up the network yet
    if (strlen(sharedData->staSSID) == 0)
    {
        return false;
    }

    // Check current connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        return true;
    }

    // SERIAL MUTEX PROTECTION: Log the reconnection attempt safely
    if (xSemaphoreTake(sharedData->serialMutex, portMAX_DELAY) == pdTRUE)
    {
        Serial.printf("\n[WiFi Task] Connection lost. Reconnecting to STA: %s\n", sharedData->staSSID);
        Serial.flush();
        xSemaphoreGive(sharedData->serialMutex);
    }

    // Trigger ESP32 internal non-blocking reconnect mechanism
    // This utilizes the credentials previously saved by the web server task
    WiFi.reconnect();

    return false;
}

void WiFi_connect(void *pvParameters) {
    // Cast the generic task parameter back to the SharedData structure pointer
    SharedData* sharedData = (SharedData*) pvParameters;

    // Main infinite loop for network monitoring
    while (1)
    {
        // Execute the reconnect function and capture the connection status
        bool isConnected = WiFi_reconnect(sharedData);

        // Update the shared state so other tasks (like CoreIOT) know the network status
        sharedData->staConnected = isConnected;

        // Check network status every 5 seconds to avoid spamming the reconnect command
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
} 