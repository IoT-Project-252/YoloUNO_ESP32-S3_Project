# Task 1 — Single LED Blink with Temperature-Conditioned PWM Behavior

## 1. Overview and Objectives

Task 1 extends the baseline LED blink functionality by coupling the blink pattern to real-time temperature readings from the DHT20 sensor. Three distinct blinking behaviors — each defined by a PWM duty cycle and a period — are mapped to three temperature ranges. Task synchronization between the sensor reading task (`temp_humi`) and the LED control task (`led_blinky`) is achieved exclusively through **FreeRTOS binary semaphores**, without introducing additional global variables.

---

## 2. Temperature Threshold Definitions

Three temperature ranges and their corresponding LED behaviors are defined as follows:

| Zone | Temperature condition | PWM duty cycle | ON period | OFF period | Semantic |
|------|----------------------|---------------|-----------|-----------|----------|
| Normal | T < 26 °C | 64 / 255 ≈ 25 % | 1 000 ms | 1 000 ms | Cool / comfortable — slow, dim blink |
| Warning | 26 °C ≤ T ≤ 28 °C | 160 / 255 ≈ 63 % | 500 ms | 500 ms | Moderate temperature — medium-rate blink |
| Critical | T > 28 °C | 255 / 255 = 100 % | 200 ms | 200 ms | High temperature — fast, full-brightness blink |

The choice of **PWM duty cycle** (rather than simple GPIO toggling) provides a perceptual intensity gradient: the LED becomes progressively brighter and faster as temperature rises, giving an intuitive multi-dimensional visual alert.

---

## 3. Semaphore-Based Synchronization Mechanism

### 3.1 Design Rationale

The assignment specification (Task 1) requires the use of semaphores and forbids creating new global variables — the LED task must consume temperature state that is produced by the sensor task. A set of **three dedicated binary semaphores** is used as event signals:

| Semaphore | Declared in | Signal meaning |
|-----------|-------------|----------------|
| `semTempNormal` | `SharedData` (`global.h`) | Current temperature < 26 °C |
| `semTempWarning` | `SharedData` (`global.h`) | 26 °C ≤ current temperature ≤ 28 °C |
| `semTempCritical` | `SharedData` (`global.h`) | Current temperature > 28 °C |

These semaphores are defined in the existing `SharedData` structure and initialized in `initGlobalData()`, reusing the shared memory object passed to all tasks — no new global variables are introduced.

### 3.2 Producer: `temp_humi` Task

After each successful DHT20 read (period: 5 000 ms), `temp_humi` evaluates the temperature and gives exactly **one** of the three semaphores:

```cpp
// src/temp_humi.cpp — threshold logic for Task 1
if (t < 26.0f) {
    xSemaphoreGive(sharedData->semTempNormal);
} else if (t <= 28.0f) {
    xSemaphoreGive(sharedData->semTempWarning);
} else {
    xSemaphoreGive(sharedData->semTempCritical);
}
```

Only one semaphore is given per reading cycle, ensuring that the consumer always receives an unambiguous, non-conflicting state update.

### 3.3 Consumer: `led_blinky` Task

`led_blinky` polls all three semaphores at the start of each blink cycle using `xSemaphoreTake` with **timeout = 0** (non-blocking `trytake`):

```cpp
// src/led_blinky.cpp
uint8_t mode = 1;   // default: Warning (26–28 °C)

while (1) {
    // Non-blocking poll — update mode only if a new signal is available
    if      (xSemaphoreTake(sharedData->semTempNormal,   0) == pdTRUE) { mode = 0; }
    else if (xSemaphoreTake(sharedData->semTempWarning,  0) == pdTRUE) { mode = 1; }
    else if (xSemaphoreTake(sharedData->semTempCritical, 0) == pdTRUE) { mode = 2; }

    // Resolve duty cycle and timing for current mode
    uint8_t  duty   = (mode == 0) ?  64 : (mode == 1) ? 160 : 255;
    uint32_t on_ms  = (mode == 0) ? 1000 : (mode == 1) ? 500 : 200;
    uint32_t off_ms = on_ms;

    // Execute one blink cycle
    ledcWrite(LED_PWM_CH, duty);
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    ledcWrite(LED_PWM_CH, 0);
    vTaskDelay(pdMS_TO_TICKS(off_ms));
}
```

**Key behavioral properties:**
- **Non-blocking poll (`timeout = 0`):** If no new temperature signal has arrived since the last cycle, `mode` retains its previous value — the LED continues blinking with the last-known temperature pattern. This prevents the task from blocking and keeps the LED alive even when the sensor is temporarily unavailable.
- **Mutual exclusivity:** Because only one semaphore is given per sensor cycle and `led_blinky` drains it immediately via `trytake`, there is no risk of semaphore accumulation or stale-state reading.
- **No shared variable for mode:** The `mode` variable is a local (stack-allocated) variable inside `led_blinky`, not a global. The temperature state is communicated purely through the semaphore mechanism, satisfying the assignment constraint.

---

## 4. PWM Configuration

The LED (GPIO 48 on YoloUNO ESP32-S3) is driven through the ESP32-S3's LEDC peripheral, which provides hardware PWM independent of software timing:

```cpp
// src/led_blinky.cpp
static const uint8_t  LED_PWM_CH   = 0;       // LEDC channel 0
static const uint32_t LED_PWM_FREQ = 5000;    // 5 kHz carrier frequency
static const uint8_t  LED_PWM_RES  = 8;       // 8-bit resolution → duty range 0..255

ledcSetup(LED_PWM_CH, LED_PWM_FREQ, LED_PWM_RES);
ledcAttachPin(LED_GPIO, LED_PWM_CH);
```

| Parameter | Value | Rationale |
|-----------|-------|-----------|
| GPIO | 48 | Built-in status LED on YoloUNO board |
| LEDC channel | 0 | Reserved for Task 1; does not conflict with other peripherals |
| Carrier frequency | 5 000 Hz | Above human flicker-fusion threshold → smooth perceived brightness |
| Duty resolution | 8 bits | Sufficient granularity (256 levels); matches `ledcWrite()` API |

The ON/OFF blinking envelope is implemented via `vTaskDelay()`, which yields the CPU during each period. The LEDC hardware maintains the PWM output autonomously during the delay, decoupling the carrier frequency from the blink timing.

---

## 5. Task Configuration and Inter-Task Data Flow

### 5.1 Task Creation (main.cpp)

```cpp
// src/main.cpp
xTaskCreate(temp_humi,   "Task Read Temperature & Humidity", 4096, projectSharedData, 1, NULL);
xTaskCreate(led_blinky,  "Task LED Blinky",                  2048, projectSharedData, 1, NULL);
```

Both tasks run at FreeRTOS priority 1. The `led_blinky` task stack is allocated at 2 048 bytes — smaller than `temp_humi` (4 096 bytes) because it performs no I2C operations or floating-point-heavy computations.

### 5.2 Shared Data Flow Diagram

```
[DHT20 Sensor] ──I2C──▶ [temp_humi task]
                              │
                    Read temperature t
                              │
                    t < 26? ──▶ xSemaphoreGive(semTempNormal)
                    26≤t≤28?──▶ xSemaphoreGive(semTempWarning)
                    t > 28? ──▶ xSemaphoreGive(semTempCritical)
                              │
                    (every 5 000 ms)
                              │
                              ▼
                    [led_blinky task] ─── polling every blink cycle
                              │
                    xSemaphoreTake(semTempNormal,   0) → mode=0
                    xSemaphoreTake(semTempWarning,  0) → mode=1
                    xSemaphoreTake(semTempCritical, 0) → mode=2
                              │
                    ledcWrite(duty) → ON → vTaskDelay(on_ms)
                    ledcWrite(0)    → OFF → vTaskDelay(off_ms)
                              │
                        [GPIO 48 / LED]
```

### 5.3 Relationship with Other Tasks

`semTempNormal`, `semTempWarning`, and `semTempCritical` are dedicated **exclusively** to Task 1. The LCD display task (Task 3) and NeoPixel task (Task 2) use the separate `semNormal`, `semWarning`, `semCritical` semaphores, also given by `temp_humi` under distinct threshold definitions. This separation prevents one consumer task from starving another.

```cpp
// src/temp_humi.cpp — LCD/Task3 semaphores (different thresholds)
if      (t >= 20 && t < 30)                         xSemaphoreGive(sharedData->semNormal);
else if ((t >= 30 && t < 38) || (t >= 7 && t < 20)) xSemaphoreGive(sharedData->semWarning);
else                                                  xSemaphoreGive(sharedData->semCritical);

// Task 1 semaphores (LED blink thresholds, spec-compliant: <26 / 26-28 / >28)
if      (t < 26.0f)   xSemaphoreGive(sharedData->semTempNormal);
else if (t <= 28.0f)  xSemaphoreGive(sharedData->semTempWarning);
else                  xSemaphoreGive(sharedData->semTempCritical);
```

---

## 6. Initialization of Semaphores (global.cpp)

All semaphores are created in `initGlobalData()` before any FreeRTOS tasks are spawned:

```cpp
// src/global.cpp
projectSharedData->semTempNormal   = xSemaphoreCreateBinary();
projectSharedData->semTempWarning  = xSemaphoreCreateBinary();
projectSharedData->semTempCritical = xSemaphoreCreateBinary();
```

Binary semaphores are initialized in the **taken** (empty) state by FreeRTOS. `led_blinky` therefore starts in `mode = 1` (Warning) by default and transitions to the correct mode as soon as the first sensor reading arrives.

---

## 7. Verification and Observed Behavior

### 7.1 Expected Serial Output

The `temp_humi` task prints the current state after each read (protected by `serialMutex`):

```
Temperature: 25.30°C | Humidity: 62.10% | State: Normal
Temperature: 27.10°C | Humidity: 65.40% | State: Normal
Temperature: 29.80°C | Humidity: 70.20% | State: Warning
```

*(Note: "State" in the serial log refers to the LCD/Task3 thresholds, not the LED blink thresholds. LED behavior is determined separately by `semTempNormal/Warning/Critical`.)*

### 7.2 Observable LED Behavior vs. Temperature

| Measured temperature | Expected LED behavior |
|---------------------|-----------------------|
| T = 24.0 °C (< 26) | Dim (duty 25 %), slow blink — 1 s ON / 1 s OFF |
| T = 27.0 °C (26–28) | Medium (duty 63 %), medium blink — 0.5 s ON / 0.5 s OFF |
| T = 30.5 °C (> 28) | Bright (duty 100 %), fast blink — 0.2 s ON / 0.2 s OFF |

### 7.3 Robustness Notes

- If the DHT20 read fails (I2C timeout or sensor error), `temp_humi` retries after 2 000 ms without giving any semaphore. `led_blinky` continues blinking with the previous `mode` value — the LED never freezes.
- Because binary semaphores do not accumulate counts, a burst of rapid temperature readings does not cause `led_blinky` to process a backlog of outdated states.

---

## 8. Discussion and Conclusion

Task 1 demonstrates a clean **producer–consumer decoupling** between sensor acquisition and LED actuation using FreeRTOS binary semaphores. The key design choices are:

1. **Semaphores as typed event signals** (one per temperature zone) rather than a shared integer variable — this satisfies the constraint of using variables from another task via semaphores, not newly declared globals.
2. **Non-blocking `trytake` (`timeout = 0`)** in the consumer — the LED task remains responsive to its own blink timing loop without being blocked by sensor latency.
3. **LEDC hardware PWM** — the carrier frequency (5 kHz) is driven by hardware, decoupling perceptual brightness from the blink envelope timing managed by `vTaskDelay()`.
4. **Dedicated semaphore sets** — Task 1 uses `semTempNormal/Warning/Critical` while Task 3/LCD uses `semNormal/Warning/Critical`, preventing cross-task semaphore interference despite both tasks consuming sensor data from the same `temp_humi` producer.

The result is a three-state, visually intuitive LED alert system that operates continuously alongside all other project tasks (NeoPixel, LCD, TinyML, Web Server, WiFi, MQTT) within the FreeRTOS scheduler, with a minimal stack footprint of 2 048 bytes.

---

## Appendix: Modified Files Summary

| File | Change description |
|------|--------------------|
| `include/global.h` | Added `semTempNormal`, `semTempWarning`, `semTempCritical` to `SharedData` struct |
| `src/global.cpp` | Initialized three binary semaphores in `initGlobalData()` |
| `src/temp_humi.cpp` | Added threshold logic to give one of three `semTemp*` semaphores after each DHT20 read |
| `src/led_blinky.cpp` | Rewrote `led_blinky()` task to poll `semTemp*` and drive PWM duty + period per temperature zone |
| `include/led_blinky.h` | `LED_GPIO = 48` pin definition |
