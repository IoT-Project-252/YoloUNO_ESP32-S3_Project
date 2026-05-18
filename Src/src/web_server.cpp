#include "web_server.h"

// ── HTML Page Builders ──────────────────────────────────────────────────────

static String buildDashboardHTML()
{
    String html = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-S3 IoT Dashboard</title>
<link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700;800&display=swap" rel="stylesheet">
<style>
:root {
  --primary: #4f46e5;
  --primary-hover: #4338ca;
  --bg-gradient: linear-gradient(135deg, #e0e7ff 0%, #ede9fe 100%);
  --card-bg: rgba(255, 255, 255, 0.7);
  --card-border: rgba(255, 255, 255, 0.5);
  --text-main: #1e293b;
  --text-muted: #64748b;
  --shadow: 0 10px 40px -10px rgba(0,0,0,0.08);
}
* { margin: 0; padding: 0; box-sizing: border-box; font-family: 'Inter', sans-serif; }
body { background: var(--bg-gradient); color: var(--text-main); min-height: 100vh; padding: 20px; display: flex; flex-direction: column; align-items: center; }
.header { width: 100%; max-width: 1000px; display: flex; justify-content: space-between; align-items: center; margin-bottom: 30px; padding: 12px 24px; background: var(--card-bg); backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px); border-radius: 20px; border: 1px solid var(--card-border); box-shadow: var(--shadow); }
.header h1 { font-size: 1.4rem; font-weight: 800; background: linear-gradient(135deg, #3b82f6, #8b5cf6); -webkit-background-clip: text; -webkit-text-fill-color: transparent; display: flex; align-items: center; gap: 8px;}
.badge { padding: 6px 14px; border-radius: 20px; background: rgba(59, 130, 246, 0.1); color: #3b82f6; font-size: 0.85rem; font-weight: 700; letter-spacing: 0.5px;}
.badge.ap { background: rgba(16, 185, 129, 0.1); color: #10b981; }
.settings-btn { text-decoration: none; color: #fff; background: var(--primary); padding: 10px 18px; border-radius: 12px; font-weight: 600; font-size: 0.9rem; transition: 0.2s; box-shadow: 0 4px 12px rgba(79, 70, 229, 0.3); display: flex; align-items: center; gap: 6px;}
.settings-btn:hover { background: var(--primary-hover); transform: translateY(-2px); box-shadow: 0 6px 16px rgba(79, 70, 229, 0.4);}
.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 24px; width: 100%; max-width: 1000px; }
.card { background: var(--card-bg); backdrop-filter: blur(16px); -webkit-backdrop-filter: blur(16px); border: 1px solid var(--card-border); border-radius: 24px; padding: 30px; box-shadow: var(--shadow); transition: transform 0.3s, box-shadow 0.3s; position: relative; overflow: hidden; }
.card:hover { transform: translateY(-4px); box-shadow: 0 20px 40px -10px rgba(0,0,0,0.12); }
.card-title { font-size: 0.85rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1px; display: flex; align-items: center; gap: 10px; margin-bottom: 24px; }
.icon { font-size: 1.4rem; }
.val-container { display: flex; align-items: baseline; gap: 4px; }
.val { font-size: 3.5rem; font-weight: 800; line-height: 1; letter-spacing: -1px;}
.unit { font-size: 1.2rem; font-weight: 700; color: var(--text-muted); }
.temp .val { color: #f97316; }
.humi .val { color: #0ea5e9; }
.bar-container { height: 8px; background: rgba(0,0,0,0.06); border-radius: 4px; margin-top: 24px; overflow: hidden; }
.bar { height: 100%; border-radius: 4px; transition: width 0.8s cubic-bezier(0.4, 0, 0.2, 1); }
.bar.temp-bar { background: linear-gradient(90deg, #fcd34d, #f97316); }
.bar.humi-bar { background: linear-gradient(90deg, #7dd3fc, #0ea5e9); }
.ctrl-group { display: flex; justify-content: space-between; align-items: center; margin-bottom: 18px; }
.ctrl-label { font-weight: 600; color: var(--text-main); font-size: 0.95rem;}
/* Toggle Switch (Apple Style) */
.switch { position: relative; display: inline-block; width: 54px; height: 30px; }
.switch input { opacity: 0; width: 0; height: 0; }
.slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #cbd5e1; transition: .4s; border-radius: 30px; box-shadow: inset 0 2px 4px rgba(0,0,0,0.05);}
.slider:before { position: absolute; content: ""; height: 24px; width: 24px; left: 3px; bottom: 3px; background-color: white; transition: .4s cubic-bezier(0.4, 0.0, 0.2, 1); border-radius: 50%; box-shadow: 0 2px 5px rgba(0,0,0,0.2); }
input:checked + .slider { background-color: #10b981; }
input:checked + .slider:before { transform: translateX(24px); }
/* Range Sliders */
.range-group { display: flex; align-items: center; gap: 14px; margin-bottom: 14px; }
.range-label { font-weight: 800; width: 14px; text-align: center; font-size: 0.9rem;}
.range-label.r { color: #ef4444; } .range-label.g { color: #22c55e; } .range-label.b { color: #3b82f6; }
input[type=range] { -webkit-appearance: none; width: 100%; background: transparent; }
input[type=range]::-webkit-slider-thumb { -webkit-appearance: none; height: 22px; width: 22px; border-radius: 50%; background: #ffffff; cursor: pointer; margin-top: -8px; box-shadow: 0 2px 6px rgba(0,0,0,0.2); border: 2px solid var(--primary); transition: transform 0.1s;}
input[type=range]::-webkit-slider-thumb:active { transform: scale(1.1); }
input[type=range]::-webkit-slider-runnable-track { width: 100%; height: 6px; cursor: pointer; background: rgba(0,0,0,0.08); border-radius: 3px; }
input[type=range]:focus { outline: none; }
.rgb-preview { width: 44px; height: 44px; border-radius: 14px; border: 2px solid rgba(255,255,255,0.8); transition: background 0.3s; box-shadow: 0 4px 10px rgba(0,0,0,0.1); }
.apply-btn { flex-grow: 1; padding: 12px; border-radius: 14px; border: none; background: linear-gradient(135deg, #8b5cf6, #6366f1); color: white; font-weight: 700; font-size: 0.95rem; cursor: pointer; transition: 0.2s; box-shadow: 0 4px 12px rgba(99, 102, 241, 0.3); }
.apply-btn:hover { opacity: 0.9; transform: translateY(-2px); box-shadow: 0 6px 16px rgba(99, 102, 241, 0.4);}
.apply-btn:active { transform: scale(0.98); }
.fan-speed-val { font-size: 2rem; font-weight: 800; color: #10b981; letter-spacing: -1px;}
.fan-speed-unit { font-size: 1.1rem; color: var(--text-muted); font-weight: 700; }
@keyframes slideUp { from { opacity: 0; transform: translateY(20px); } to { opacity: 1; transform: translateY(0); } }
.card { animation: slideUp 0.6s cubic-bezier(0.16, 1, 0.3, 1) both; }
.card:nth-child(1) { animation-delay: 0.1s; } .card:nth-child(2) { animation-delay: 0.2s; }
.card:nth-child(3) { animation-delay: 0.3s; } .card:nth-child(4) { animation-delay: 0.4s; }
</style>
</head>
<body>
<div class="header">
  <h1><span class="icon">&#9889;</span> YoloUNO</h1>
  <div style="display: flex; gap: 16px; align-items: center;">
    <span class="badge" id="staBadge">Loading...</span>
    <a href="/settings" class="settings-btn">&#9881;&#65039; Settings</a>
  </div>
</div>
<div class="grid">
  <!-- Temp -->
  <div class="card temp">
    <div class="card-title"><span class="icon">&#127777;&#65039;</span> Temperature</div>
    <div class="val-container"><span class="val" id="temp">--</span><span class="unit">&deg;C</span></div>
    <div class="bar-container"><div class="bar temp-bar" id="tempBar" style="width: 0%"></div></div>
  </div>
  <!-- Humi -->
  <div class="card humi">
    <div class="card-title"><span class="icon">&#128167;</span> Humidity</div>
    <div class="val-container"><span class="val" id="humi">--</span><span class="unit">%</span></div>
    <div class="bar-container"><div class="bar humi-bar" id="humiBar" style="width: 0%"></div></div>
  </div>
  <!-- LED -->
  <div class="card">
    <div class="card-title"><span class="icon">&#128161;</span> Smart LED</div>
    <div class="ctrl-group">
      <span class="ctrl-label">Power</span>
      <label class="switch"><input type="checkbox" id="ledSw" onchange="ctrlLed()"><span class="slider"></span></label>
    </div>
    <div class="range-group"><span class="range-label r">R</span><input type="range" min="0" max="255" value="0" id="rS" oninput="document.getElementById('rV').textContent=this.value; updatePreview()" onchange="ctrlLed()"><span id="rV" style="width:28px;text-align:right;font-size:.85rem;font-weight:600;color:#64748b">0</span></div>
    <div class="range-group"><span class="range-label g">G</span><input type="range" min="0" max="255" value="0" id="gS" oninput="document.getElementById('gV').textContent=this.value; updatePreview()" onchange="ctrlLed()"><span id="gV" style="width:28px;text-align:right;font-size:.85rem;font-weight:600;color:#64748b">0</span></div>
    <div class="range-group"><span class="range-label b">B</span><input type="range" min="0" max="255" value="0" id="bS" oninput="document.getElementById('bV').textContent=this.value; updatePreview()" onchange="ctrlLed()"><span id="bV" style="width:28px;text-align:right;font-size:.85rem;font-weight:600;color:#64748b">0</span></div>
    <div style="display: flex; justify-content: center; margin-top: 24px;">
      <div class="rgb-preview" id="ledPreview" style="width: 100%; height: 48px; border-radius: 16px;"></div>
    </div>
  </div>
  <!-- Fan -->
  <div class="card">
    <div class="card-title"><span class="icon">&#128168;</span> Cooling Fan</div>
    <div class="ctrl-group">
      <span class="ctrl-label">Power</span>
      <label class="switch"><input type="checkbox" id="fanSw" onchange="ctrlFan()"><span class="slider"></span></label>
    </div>
    <div class="ctrl-group" style="margin-top: 24px;">
      <span class="ctrl-label">Speed Level</span>
      <div class="val-container"><span class="fan-speed-val" id="fanV">0</span><span class="fan-speed-unit">%</span></div>
    </div>
    <input type="range" min="0" max="100" value="0" id="fanS" oninput="document.getElementById('fanV').textContent=this.value" onchange="ctrlFan()" style="margin-top: 14px;">
  </div>
</div>
<script>
function fetchState(){
  fetch('/api/state').then(r=>r.json()).then(d=>{
    document.getElementById('temp').textContent=d.temperature.toFixed(1);
    document.getElementById('humi').textContent=d.humidity.toFixed(1);
    document.getElementById('tempBar').style.width=Math.min(100, Math.max(0, (d.temperature/50)*100))+'%';
    document.getElementById('humiBar').style.width=Math.min(100, Math.max(0, d.humidity))+'%';
    document.getElementById('ledSw').checked=d.ledOn;
    document.getElementById('rS').value=d.ledR; document.getElementById('rV').textContent=d.ledR;
    document.getElementById('gS').value=d.ledG; document.getElementById('gV').textContent=d.ledG;
    document.getElementById('bS').value=d.ledB; document.getElementById('bV').textContent=d.ledB;
    document.getElementById('fanSw').checked=d.fanOn;
    document.getElementById('fanS').value=d.fanSpeed; document.getElementById('fanV').textContent=d.fanSpeed;
    
    let b=document.getElementById('staBadge');
    if(d.staConnected){ b.textContent='STA: '+d.staSSID; b.className='badge'; }
    else{ b.textContent='AP Mode'; b.className='badge ap'; }
    updatePreview();
  }).catch(e=>console.error(e));
}
function updatePreview(){
  let r=document.getElementById('rS').value, g=document.getElementById('gS').value, b=document.getElementById('bS').value;
  let on=document.getElementById('ledSw').checked;
  document.getElementById('ledPreview').style.background = on ? `rgb(${r},${g},${b})` : '#cbd5e1';
}
function ctrlLed(){
  let on=document.getElementById('ledSw').checked?1:0;
  let r=document.getElementById('rS').value, g=document.getElementById('gS').value, b=document.getElementById('bS').value;
  fetch(`/api/led?on=${on}&r=${r}&g=${g}&b=${b}`).then(()=>updatePreview());
}
function ctrlFan(){
  let on=document.getElementById('fanSw').checked?1:0;
  let s=document.getElementById('fanS').value;
  fetch(`/api/fan?on=${on}&speed=${s}`);
}
setInterval(fetchState, 3000); fetchState();
</script>
</body>
</html>
)rawliteral";
    return html;
}

static String buildSettingsHTML()
{
    String html = R"rawliteral(
<!DOCTYPE html><html lang="en"><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>WiFi Settings</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:'Segoe UI',Tahoma,Geneva,Verdana,sans-serif;background:linear-gradient(135deg,#f0f4ff 0%,#e8edfb 50%,#f5f0ff 100%);color:#1e293b;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.box{background:#fff;border-radius:24px;padding:40px 36px;width:100%;max-width:440px;box-shadow:0 8px 40px rgba(99,102,241,.08);border:1px solid #e8ecf4}
h1{font-size:1.4rem;margin-bottom:8px;text-align:center;font-weight:700;background:linear-gradient(135deg,#6366f1,#06b6d4);-webkit-background-clip:text;-webkit-text-fill-color:transparent}
.subtitle{text-align:center;font-size:.85rem;color:#94a3b8;margin-bottom:28px}
label{display:block;font-size:.85rem;font-weight:600;color:#475569;margin-bottom:6px;margin-top:18px;text-transform:uppercase;letter-spacing:.04em}
.input-wrap{position:relative}
.input-wrap .ico{position:absolute;left:14px;top:50%;transform:translateY(-50%);font-size:1rem;color:#94a3b8}
input[type=text],input[type=password]{width:100%;padding:12px 14px 12px 42px;border-radius:14px;border:1.5px solid #e2e8f0;background:#f8fafc;color:#1e293b;font-size:1rem;outline:none;transition:border-color .2s,box-shadow .2s}
input:focus{border-color:#6366f1;box-shadow:0 0 0 3px rgba(99,102,241,.1)}
button{margin-top:26px;width:100%;padding:14px;border:none;border-radius:14px;background:linear-gradient(135deg,#6366f1,#8b5cf6);color:#fff;font-size:1rem;font-weight:700;cursor:pointer;transition:all .2s;box-shadow:0 4px 16px rgba(99,102,241,.2)}
button:hover{opacity:.92;box-shadow:0 6px 20px rgba(99,102,241,.3)}
button:active{transform:scale(.98)}
#msg{margin-top:18px;text-align:center;font-size:.9rem;min-height:1.4em;padding:8px;border-radius:10px}
.ok{color:#16a34a;background:#f0fdf4;border:1px solid #bbf7d0}
.err{color:#dc2626;background:#fef2f2;border:1px solid #fecaca}
.info{color:#d97706;background:#fffbeb;border:1px solid #fde68a}
.nav{text-align:center;margin-top:22px}
.nav a{color:#6366f1;text-decoration:none;font-size:.9rem;font-weight:500;padding:8px 16px;border-radius:10px;background:#f0f0ff;transition:all .2s;display:inline-block}
.nav a:hover{background:#e0e0ff}
@keyframes spin{to{transform:rotate(360deg)}}
.spinner{display:inline-block;width:16px;height:16px;border:2px solid #d97706;border-top-color:transparent;border-radius:50%;animation:spin .6s linear infinite;vertical-align:middle;margin-right:6px}
</style></head><body>
<div class="box">
<h1>&#128274; WiFi Station Settings</h1>
<div class="subtitle">Connect your ESP32 to a WiFi network</div>
<label for="ssid">Network Name (SSID)</label>
<div class="input-wrap"><span class="ico">&#128246;</span><input type="text" id="ssid" placeholder="Enter WiFi SSID" maxlength="32"></div>
<label for="pass">Password</label>
<div class="input-wrap"><span class="ico">&#128272;</span><input type="password" id="pass" placeholder="Enter WiFi Password" maxlength="64"></div>
<button onclick="saveWifi()" id="btn">Save &amp; Connect</button>
<div id="msg"></div>
<div class="nav"><a href="/">&#8592; Back to Dashboard</a></div>
</div>
<script>
function saveWifi(){
  var ssid=document.getElementById('ssid').value.trim();
  var pass=document.getElementById('pass').value;
  var m=document.getElementById('msg');
  if(!ssid){m.className='err';m.innerHTML='SSID is required!';return;}
  m.className='info';m.innerHTML='<span class="spinner"></span>Connecting to <b>'+ssid+'</b>...';
  document.getElementById('btn').disabled=true;
  fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid:ssid,password:pass})})
  .then(r=>r.json()).then(d=>{
    document.getElementById('btn').disabled=false;
    if(d.success){m.className='ok';m.innerHTML='&#9989; Connected to <b>'+d.ssid+'</b> (IP: '+d.ip+')';setTimeout(()=>{window.location='/';},2500);}
    else{m.className='err';m.innerHTML='&#10060; '+d.message;}
  }).catch(e=>{document.getElementById('btn').disabled=false;m.className='err';m.innerHTML='&#10060; Request error: '+e;});
}
</script></body></html>
)rawliteral";
    return html;
}

// ── Async Web Server instance (port 80) ─────────────────────────────────────
static AsyncWebServer server(80);

// ── Route Handlers ──────────────────────────────────────────────────────────

static void setupRoutes(SharedData* sd)
{
    // --- Dashboard ---
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/html", buildDashboardHTML());
    });

    // --- Settings page ---
    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *req) {
        req->send(200, "text/html", buildSettingsHTML());
    });

    // --- API: get full state ---
    server.on("/api/state", HTTP_GET, [sd](AsyncWebServerRequest *req) {
        StaticJsonDocument<256> doc;

        if (xSemaphoreTake(sd->mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            doc["temperature"] = sd->temperature;
            doc["humidity"]    = sd->humidity;
            xSemaphoreGive(sd->mutex);
        } else {
            doc["temperature"] = 0; doc["humidity"] = 0;
        }

        if (xSemaphoreTake(sd->actuatorMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            doc["ledOn"]    = sd->ledOn;
            doc["ledR"]     = sd->ledR;
            doc["ledG"]     = sd->ledG;
            doc["ledB"]     = sd->ledB;
            doc["fanOn"]    = sd->fanOn;
            doc["fanSpeed"] = sd->fanSpeed;
            xSemaphoreGive(sd->actuatorMutex);
        }

        doc["staConnected"] = sd->staConnected;
        doc["staSSID"]      = sd->staSSID;

        String out;
        serializeJson(doc, out);
        req->send(200, "application/json", out);
    });

    // --- API: control LED ---
    server.on("/api/led", HTTP_GET, [sd](AsyncWebServerRequest *req) {
        if (xSemaphoreTake(sd->actuatorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (req->hasParam("on"))  sd->ledOn = req->getParam("on")->value().toInt();
            if (req->hasParam("r"))   sd->ledR  = constrain(req->getParam("r")->value().toInt(), 0, 255);
            if (req->hasParam("g"))   sd->ledG  = constrain(req->getParam("g")->value().toInt(), 0, 255);
            if (req->hasParam("b"))   sd->ledB  = constrain(req->getParam("b")->value().toInt(), 0, 255);
            sd->ledChanged = true;
            xSemaphoreGive(sd->actuatorMutex);
        }
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // --- API: control Fan ---
    server.on("/api/fan", HTTP_GET, [sd](AsyncWebServerRequest *req) {
        if (xSemaphoreTake(sd->actuatorMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (req->hasParam("on"))    sd->fanOn    = req->getParam("on")->value().toInt();
            if (req->hasParam("speed")) sd->fanSpeed = constrain(req->getParam("speed")->value().toInt(), 0, 100);
            sd->fanChanged = true;
            xSemaphoreGive(sd->actuatorMutex);
        }
        req->send(200, "application/json", "{\"ok\":true}");
    });

    // --- API: save WiFi credentials & connect STA ---
    server.on("/api/wifi", HTTP_POST,
        [](AsyncWebServerRequest *req) { /* body handler below sends response */ },
        NULL,
        [sd](AsyncWebServerRequest *req, uint8_t *data, size_t len, size_t index, size_t total) {
            // Accumulate body
            String body;
            body.reserve(total);
            for (size_t i = 0; i < len; i++) body += (char)data[i];

            StaticJsonDocument<256> doc;
            DeserializationError err = deserializeJson(doc, body);
            if (err) {
                req->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
                return;
            }

            const char* ssid = doc["ssid"] | "";
            const char* pass = doc["password"] | "";

            if (strlen(ssid) == 0) {
                req->send(400, "application/json", "{\"success\":false,\"message\":\"SSID is required\"}");
                return;
            }

            Serial.printf("[WiFi] Attempting STA connection to: %s\n", ssid);

            // Switch to AP+STA mode
            WiFi.mode(WIFI_AP_STA);
            WiFi.begin(ssid, pass);

            // Wait for connection (max 15 seconds, non-blocking with small delays)
            int attempts = 0;
            while (WiFi.status() != WL_CONNECTED && attempts < 30) {
                vTaskDelay(pdMS_TO_TICKS(500));
                attempts++;
                Serial.print(".");
            }
            Serial.println();

            StaticJsonDocument<200> resp;
            if (WiFi.status() == WL_CONNECTED) {
                sd->staConnected = true;
                strncpy(sd->staSSID, ssid, sizeof(sd->staSSID) - 1);
                sd->staSSID[sizeof(sd->staSSID) - 1] = '\0';

                resp["success"] = true;
                resp["ssid"]    = ssid;
                resp["ip"]      = WiFi.localIP().toString();
                Serial.printf("[WiFi] STA connected! IP: %s\n", WiFi.localIP().toString().c_str());
            } else {
                WiFi.disconnect(true);
                // Stay in AP mode if STA fails
                WiFi.mode(WIFI_AP);
                sd->staConnected = false;
                memset(sd->staSSID, 0, sizeof(sd->staSSID));

                resp["success"] = false;
                resp["message"] = "Connection failed. Check SSID/password.";
                Serial.println("[WiFi] STA connection failed.");
            }

            String out;
            serializeJson(resp, out);
            req->send(200, "application/json", out);
        }
    );
}

// ── Web Server FreeRTOS Task ────────────────────────────────────────────────
void webServerTask(void *pvParameters)
{
    SharedData* sd = (SharedData*) pvParameters;

    // --- Start AP mode ---
    WiFi.mode(WIFI_AP);
    WiFi.softAP(SSID_AP, PASS_AP);
    Serial.println("[WebServer] AP started.");
    Serial.print("[WebServer] AP IP: ");
    Serial.println(WiFi.softAPIP());

    // --- Setup routes and start server ---
    setupRoutes(sd);
    server.begin();
    Serial.println("[WebServer] HTTP server started on port 80.");

    // Keep task alive (AsyncWebServer runs in background via ESP-IDF)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
