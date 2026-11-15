#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------- Налаштування ----------
// Сенсор TTP223B: VCC -> 3V3, GND -> GND, OUT -> GPIO4
static const int TOUCH_PIN = 4;

static const char *AP_SSID = "ESP32-Counter";
static const char *AP_PASS = "12345678";  // мінімум 8 символів

// ---------- Стан ----------
static uint32_t counter = 0;
static bool rawState = false;
static bool stableState = false;
static unsigned long lastEdgeMs = 0;
static const unsigned long DEBOUNCE_MS = 30;

WebServer server(80);

// ---------- Сторінка ----------
static const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Сенсорний лічильник</title>
<style>
  :root { --bg:#0f1220; --card:#181c2f; --accent:#5eead4; --muted:#8b93b0; }
  * { box-sizing:border-box; -webkit-tap-highlight-color:transparent; }
  body {
    margin:0; min-height:100vh; display:flex; align-items:center; justify-content:center;
    background:radial-gradient(1200px 600px at 50% -10%, #1d2340 0%, var(--bg) 60%);
    color:#eef1ff; font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
    padding:24px;
  }
  .card {
    width:min(420px,100%); background:var(--card); border-radius:24px; padding:32px 28px 28px;
    box-shadow:0 20px 60px rgba(0,0,0,.45), inset 0 1px 0 rgba(255,255,255,.06);
    text-align:center;
  }
  h1 { font-size:15px; letter-spacing:.14em; text-transform:uppercase; color:var(--muted); margin:0 0 8px; font-weight:600; }
  .counter {
    font-size:clamp(72px,26vw,120px); font-weight:800; line-height:1;
    font-variant-numeric:tabular-nums; letter-spacing:-.03em;
    background:linear-gradient(180deg,#ffffff,#9fb0ff);
    -webkit-background-clip:text; background-clip:text; color:transparent;
    transition:transform .12s ease; user-select:none;
  }
  .counter.pop { transform:scale(1.07); }
  .hint { color:var(--muted); font-size:14px; margin:14px 0 22px; line-height:1.5; }
  .dot { display:inline-block; width:8px; height:8px; border-radius:50%; background:#f59e0b; margin-right:7px; vertical-align:middle; }
  .dot.online { background:var(--accent); box-shadow:0 0 10px var(--accent); }
  .status { font-size:13px; color:var(--muted); }
  button {
    margin-top:20px; width:100%; padding:14px 18px; border-radius:14px; border:1px solid rgba(255,255,255,.12);
    background:rgba(255,255,255,.04); color:#eef1ff; font-size:15px; font-weight:600; cursor:pointer;
  }
  button:active { background:rgba(255,255,255,.1); }
  .footer { margin-top:18px; font-size:12px; color:#5b6482; }
</style>
</head>
<body>
  <main class="card">
    <h1>Сенсорний лічильник</h1>
    <div class="counter" id="count">0</div>
    <p class="hint">Торкніться сенсора TTP223B<br>кожен дотик додає +1</p>
    <div class="status"><span class="dot" id="dot"></span><span id="status">з'єднання…</span></div>
    <button id="reset">Скинути лічильник</button>
    <div class="footer">ESP32 · GPIO4</div>
  </main>
<script>
  const el = document.getElementById('count');
  const dot = document.getElementById('dot');
  const statusEl = document.getElementById('status');
  let shown = 0;
  let fails = 0;

  function render(value) {
    if (value === shown) return;
    shown = value;
    el.textContent = value;
    el.classList.add('pop');
    setTimeout(() => el.classList.remove('pop'), 120);
  }

  async function poll() {
    try {
      const r = await fetch('/api/state', { cache: 'no-store' });
      const data = await r.json();
      render(data.count);
      fails = 0;
      dot.classList.add('online');
      statusEl.textContent = data.touch ? 'сенсор: дотик' : 'сенсор: готовий';
    } catch (e) {
      fails++;
      if (fails > 2) {
        dot.classList.remove('online');
        statusEl.textContent = "немає зв'язку з платою";
      }
    }
  }

  document.getElementById('reset').addEventListener('click', async () => {
    try { await fetch('/api/reset', { cache: 'no-store' }); } catch (e) {}
    poll();
  });

  poll();
  setInterval(poll, 250);
</script>
</body>
</html>
)rawliteral";

// ---------- HTTP ----------
void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", PAGE);
}

void handleState() {
  String json = "{\"count\":";
  json += counter;
  json += ",\"touch\":";
  json += stableState ? "true" : "false";
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleReset() {
  counter = 0;
  Serial.println("[http] counter reset");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", "{\"ok\":true}");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("=== ESP32 touch counter ===");

  pinMode(TOUCH_PIN, INPUT);

  WiFi.mode(WIFI_AP);
  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP '%s' %s, IP: %s\n", AP_SSID, ok ? "started" : "FAILED",
                WiFi.softAPIP().toString().c_str());
  Serial.printf("Відкрийте у браузері: http://%s\n", WiFi.softAPIP().toString().c_str());

  server.on("/", handleRoot);
  server.on("/api/state", handleState);
  server.on("/api/reset", handleReset);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();

  bool raw = digitalRead(TOUCH_PIN) == HIGH;
  if (raw != rawState) {
    rawState = raw;
    if (millis() - lastEdgeMs > DEBOUNCE_MS) {
      lastEdgeMs = millis();
      stableState = raw;
      if (raw) {  // фронт дотику
        counter++;
        Serial.printf("touch -> count=%lu\n", (unsigned long)counter