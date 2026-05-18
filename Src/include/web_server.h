#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "global.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// ── Web Server Task ─────────────────────────────────────────────────────────
void webServerTask(void *pvParameters);

#endif
