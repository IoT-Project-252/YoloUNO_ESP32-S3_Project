#include "actuators.h"

// ── Actuator Task ───────────────────────────────────────────────────────────
// Polls SharedData for changes and drives the NeoPixel LED and fan motor.
void actuatorTask(void *pvParameters)
{
    SharedData* sd = (SharedData*) pvParameters;

    // --- NeoPixel Setup ---
    Adafruit_NeoPixel strip(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
    strip.begin();
    strip.clear();
    strip.show();
    Serial.println("[Actuator] NeoPixel initialized.");

    // --- Fan PWM Setup (LEDC) ---
    ledcSetup(FAN_PWM_CHANNEL, FAN_PWM_FREQ, FAN_PWM_RES);
    ledcAttachPin(FAN_PIN, FAN_PWM_CHANNEL);
    ledcWrite(FAN_PWM_CHANNEL, 0);
    Serial.println("[Actuator] Fan PWM initialized.");

    // Local cache to avoid redundant writes
    bool     prevLedOn = false;
    uint8_t  prevR = 0, prevG = 0, prevB = 0;
    bool     prevFanOn = false;
    uint8_t  prevFanSpeed = 0;

    while (1)
    {
        // --- Read actuator state under mutex ---
        bool    curLedOn;
        uint8_t curR, curG, curB;
        bool    curFanOn;
        uint8_t curFanSpeed;
        bool    needLed = false, needFan = false;

        if (xSemaphoreTake(sd->actuatorMutex, pdMS_TO_TICKS(50)) == pdTRUE)
        {
            curLedOn   = sd->ledOn;
            curR       = sd->ledR;
            curG       = sd->ledG;
            curB       = sd->ledB;
            curFanOn   = sd->fanOn;
            curFanSpeed = sd->fanSpeed;

            if (sd->ledChanged) { needLed = true; sd->ledChanged = false; }
            if (sd->fanChanged) { needFan = true; sd->fanChanged = false; }

            xSemaphoreGive(sd->actuatorMutex);
        }

        // --- Drive NeoPixel ---
        if (needLed ||
            curLedOn != prevLedOn || curR != prevR || curG != prevG || curB != prevB)
        {
            if (curLedOn) {
                strip.setPixelColor(0, strip.Color(curR, curG, curB));
            } else {
                strip.setPixelColor(0, 0);
            }
            strip.show();
            prevLedOn = curLedOn;
            prevR = curR; prevG = curG; prevB = curB;
            Serial.printf("[Actuator] LED %s  R:%d G:%d B:%d\n",
                          curLedOn ? "ON" : "OFF", curR, curG, curB);
        }

        // --- Drive Fan PWM ---
        if (needFan || curFanOn != prevFanOn || curFanSpeed != prevFanSpeed)
        {
            uint32_t duty = 0;
            if (curFanOn) {
                duty = map(curFanSpeed, 0, 100, 0, 255);
            }
            ledcWrite(FAN_PWM_CHANNEL, duty);
            prevFanOn = curFanOn;
            prevFanSpeed = curFanSpeed;
            Serial.printf("[Actuator] Fan %s  Speed:%d%%  Duty:%lu\n",
                          curFanOn ? "ON" : "OFF", curFanSpeed, duty);
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
