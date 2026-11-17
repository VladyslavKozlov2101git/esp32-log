#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------- Налаштування ----------
static const int TRIG_PIN = 18;  // HC-SR04 Trig (3.3 В з ESP32 йому досить)
static const int ECHO_PIN = 19;  // HC-SR04 Echo через дільник 5 В -> 3.3 В
static const int LED_PIN = 21;   // KY-034 S

static const char *AP_SSID = "ESP32-Parktronic";
static const char *AP_PASS = "12345678";

// Звук проходить 1 см туди й назад приблизно за 58 мкс (при ~20 °C).
// Калібрування: поставте предмет на відоме місце (лінійка) і підправте число.
static const float US_PER_CM = 58.0f;
static const float NEAR_CM = 10.0f;            // ближче — KY-034 світить безперервно
static const float FAR_CM = 100.0f;            // далі — не блимає
static const unsigned long MEASURE_MS = 60;    // HC-SR04 не любить частіше ніж ~60 мс
static const unsigned long ECHO_TIMEOUT_US = 25000;  // ~4 м; довше — відлуння немає

// ---------- Стан ----------
static float distanceCm = -1;  // -1 = відлуння не прийшло (нічого немає або задалеко)
static unsigned long lastMeasure = 0;
static unsigned long lastLog = 0;

WebServer server(80);

// ---------- Сторінка ----------
static const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Парктронік · ESP32</title>
<style>
  :root { --bg:#0b0f18; --card:#141a29; --line:#232c42; --muted:#8892ab; --ok:#34d399; --mid:#fbbf24; --bad:#f87171; }
  * { box-sizing:border-box; }
  body { margin:0; min-height:100vh; padding:20px; display:flex; justify-content:center;
    background:radial-gradient(900px 500px at 50% -15%, #18233b 0%, var(--bg) 65%);
    color:#e8eefc; font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif; }
  .wrap { width:min(520px,100%); }
  .card { background:var(--card); border:1px solid var(--line); border-radius:20px; padding:24px; box-shadow:0 18px 50px rgba(0,0,0,.45); }
  h1 { font-size:13px; letter-spacing:.16em; text-transform:uppercase; color:var(--muted); margin:0 0 10px; font-weight:700; }
  .num { font-size:72px; font-weight:800; line-height:1; font-variant-numeric:tabular-nums; }
  .num small { font-size:24px; color:var(--muted); font-weight:600; margin-left:6px; }
  .zone { margin-top:8px; font-weight:700; font-size:15px; }
  .road { position:relative; height:70px; margin-top:22px; border-radius:12px; background:rgba(255,255,255,.04); border:1px solid var(--line); overflow:hidden; }
  .wall { position:absolute; right:0; top:0; bottom:0; width:10px; background:repeating-linear-gradient(45deg,#f87171 0 6px,#fde68a 6px 12px); }
  .car { position:absolute; top:18px; width:58px; height:34px; border-radius:10px 16px 8px 8px; background:currentColor; transition:right .15s linear; }
  .ticks { display:flex; justify-content:space-between; font-size:11px; color:var(--muted); margin-top:6px; font-variant-numeric:tabular-nums; }
  .row { display:flex; align-items:center; gap:10px; margin-top:16px; font-size:13px; color:var(--muted); }
  .dot { width:9px; height:9px; border-radius:50%; background:#f59e0b; }
  .dot.online { background:var(--ok); box-shadow:0 0 10px var(--ok); }
</style>
</head>
<body>
<div class="wrap">
  <div class="card">
    <h1>Парктронік · HC-SR04</h1>
    <div class="num" id="num">—<small>см</small></div>
    <div class="zone" id="zone">чекаю на дані…</div>
    <div class="road"><div class="car" id="car"></div><div class="wall"></div></div>
    <div class="ticks"><span>200 см</span><span>150</span><span>100</span><span>50</span><span>0</span></div>
    <div class="row"><span class="dot" id="dot"></span><span id="status">з'єднання…</span></div>
  </div>
</div>
<script>
const num = document.getElementById('num'), zone = document.getElementById('zone');
const car = document.getElementById('car'), dot = document.getElementById('dot'), statusEl = document.getElementById('status');
let fails = 0;
function show(cm) {
  if (cm === null) {
    num.innerHTML = '—<small>см</small>';
    zone.textContent = 'нічого немає (або далі ~4 м)'; zone.style.color = 'var(--muted)';
    car.style.right = 'calc(100% - 58px)'; car.style.color = 'var(--muted)';
    return;
  }
  num.innerHTML = Math.round(cm) + '<small>см</small>';
  const [txt, col] = cm < 10 ? ['СТОП!', 'var(--bad)'] : cm < 30 ? ['близько', 'var(--bad)']
                  : cm < 100 ? ['обережно', 'var(--mid)'] : ['вільно', 'var(--ok)'];
  zone.textContent = txt; zone.style.color = col; car.style.color = col;
  const k = Math.min(cm, 200) / 200;   // 0 = впритул до стіни, 1 = 2 м і далі
  car.style.right = 'calc(10px + (100% - 68px) * ' + k.toFixed(3) + ')';
}
async function poll() {
  try {
    const r = await fetch('/api/state', { cache:'no-store' });
    const d = await r.json();
    show(d.cm);
    fails = 0; dot.className = 'dot online'; statusEl.textContent = 'на зв\'язку';
  } catch (e) {
    if (++fails > 2) { dot.className = 'dot'; statusEl.textContent = "немає зв'язку"; }
  }
}
poll();
setInterval(poll, 150);
</script>
</body>
</html>
)rawliteral";

// ---------- Вимірювання ----------
float measureCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);  // імпульс 10 мкс = «крикни»
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long us = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);  // скільки летіло відлуння
  if (us == 0) return -1;
  return us / US_PER_CM;
}

// Чим ближче предмет, тим частіше блимає KY-034.
void updateLed(unsigned long now) {
  if (distanceCm < 0 || distanceCm >= FAR_CM) {
    digitalWrite(LED_PIN, LOW);
    return;
  }
  if (distanceCm <= NEAR_CM) {
    digitalWrite(LED_PIN, HIGH);
    return;
  }
  // 10 см -> період 80 мс (часто), 100 см -> 800 мс (рідко)
  unsigned long period = 80 + (unsigned long)((distanceCm - NEAR_CM) * (800 - 80) / (FAR_CM - NEAR_CM));
  digitalWrite(LED_PIN, (now % period) < period / 2 ? HIGH : LOW);
}

// ---------- HTTP ----------
void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", PAGE);
}

void handleState() {
  String json = "{\"cm\":";
  json += distanceCm < 0 ? String("null") : String(distanceCm, 1);
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("=== ESP32 parktronic ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP '%s' %s, IP: %s\n", AP_SSID, ok ? "started" : "FAILED",
                WiFi.softAPIP().toString().c_str());

  server.on("/", handleRoot);
  server.on("/api/state", handleState);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();
  unsigned long now = millis();

  if (now - lastMeasure >= MEASURE_MS) {
    lastMeasure = now;
    distanceCm = measureCm();
  }
  updateLed(now);

  if (now - lastLog >= 500) {
    lastLog = now;
    if (distanceCm < 0) Serial.println("no echo");
    else Serial.printf("%.1f cm\n", distanceCm);
  }
}
