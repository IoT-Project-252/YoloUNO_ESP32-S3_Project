#include <Arduino.h>
#include <cstdarg>
#include <cstdio>

extern "C" void DebugLog(const char* format, va_list args) {
    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);
    Serial.print(buffer);
}
