#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------- Піни (парктронік з уроку 3 + серво) ----------
static const int TRIG_PIN = 18;   // HC-SR04 Trig
static const int ECHO_PIN = 19;   // HC-SR04 Echo через дільник 5 В -> 3.3 В
static const int LED_PIN = 21;    // KY-034 S
static const int SERVO_PIN = 23;  // SG90, помаранчевий дріт

static const char *AP_SSID = "ESP32-Radar";
static const char *AP_PASS = "12345678";

// ---------- Серво: калібрування під свій SG90 ----------
// Якщо серво не доходить до краю — зменшіть MIN / збільшіть MAX (крок 50 мкс).
// Якщо на краю дзижчить і впирається — навпаки.
static const int SERVO_MIN_US = 500;    // імпульс для 0°
static const int SERVO_MAX_US = 2400;   // імпульс для 180°
static const int SWEEP_MIN = 15;        // межі огляду: на 0° і 180° SG90 часто впирається
static const int SWEEP_MAX = 165;
static const int STEP_DEG = 3;          // крок повороту
static const unsigned long STEP_MS = 70;  // поворот + заспокоєння + вимір; HC-SR04 хоче >= 60 мс

// ---------- Далекомір ----------
static const float US_PER_CM = 58.0f;
static const float MIN_VALID_CM = 3.0f;  // ближче — це «сліпе» проникнення, а не ціль              // калібрування за лінійкою, як у парктроніку
static const unsigned long ECHO_TIMEOUT_US = 25000;  // ~4 м
static const float ALERT_CM = 30.0f;               // звичайний режим: ближче — KY-034 світить

// ---------- Охорона ----------
static const int LEARN_PASSES = 2;                 // скільки повних проходів запам'ятовувати «порожню кімнату»
static const int TOL_CM = 15;                      // на скільки ближче за норму — вже «щось нове»
static const int GUARD_MAX_CM = 250;               // де в нормі було порожньо, реагувати лише ближче цього
static const int CONFIRM_STEPS = 3;                // тривога, коли «нове» на стількох кутах підряд (відсіює стрибки датчика)
static const unsigned long ALARM_HOLD_MS = 3000;   // скільки світить KY-034 після тривоги

// ---------- Стан ----------
static const int N = (SWEEP_MAX - SWEEP_MIN) / STEP_DEG + 1;  // точок за один прохід
static int scanCm[N];       // відстань для кожного кута, -1 = відлуння немає
static int idx = 0;         // поточна точка
static int dir = 1;         // 1 = вліво (кут росте), -1 = вправо
static float lastCm = -1;
static unsigned long lastStep = 0;
static uint32_t sweeps = 0;

enum GuardState { G_OFF, G_LEARN, G_ARMED };
static GuardState guard = G_OFF;
static int learnLeft = 0;            // скільки ще проходів учитися
static int baseCm[N];                // «норма» для кожного кута, -1 = порожньо
static bool changed[N];              // на цьому куті зараз ближче за норму
static int streak = 0;               // скільки кутів підряд «щось нове»
static unsigned long alarmUntil = 0;
static int alarmAngle = -1;
static uint32_t alarms = 0;

WebServer server(80);

// ---------- Сторінка ----------
static const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Радар · ESP32</title>
<style>
  :root { --bg:#04110b; --card:#071a11; --line:#0f3a24; --green:#3dff8f; --muted:#5f9a7a; --amber:#ffc845; --red:#ff5d5d; }
  * { box-sizing:border-box; }
  body { margin:0; min-height:100vh; padding:16px; display:flex; justify-content:center; background:var(--bg); color:#d8ffe9;
    font-family:ui-monospace,SFMono-Regular,Consolas,monospace; }
  .wrap { width:min(720px,100%); }
  .card { background:var(--card); border:1px solid var(--line); border-radius:18px; padding:16px; box-shadow:0 0 40px rgba(61,255,143,.08); }
  h1 { font-size:13px; letter-spacing:.2em; text-transform:uppercase; color:var(--muted); margin:0 0 10px; }
  canvas { width:100%; display:block; }
  .read { display:flex; gap:18px; flex-wrap:wrap; align-items:baseline; margin-top:10px; }
  .big { font-size:34px; font-weight:700; color:var(--green); font-variant-numeric:tabular-nums; }
  .muted { color:var(--muted); font-size:13px; }
  .ctl { display:flex; gap:14px; flex-wrap:wrap; align-items:center; margin-top:12px; font-size:13px; color:var(--muted); }
  select, label { color:#d8ffe9; font:inherit; }
  select { background:#0b2417; border:1px solid var(--line); border-radius:8px; padding:4px 8px; }
  .dot { width:9px; height:9px; border-radius:50%; background:#f59e0b; display:inline-block; margin-right:6px; }
  .dot.online { background:var(--green); box-shadow:0 0 10px var(--green); }
  .banner { margin-top:10px; padding:10px 12px; border-radius:10px; font-size:14px; font-weight:700; display:none; }
  .banner.learn { display:block; background:#2a2208; color:var(--amber); border:1px solid #5a4710; }
  .banner.calm { display:block; background:#0b2417; color:var(--green); border:1px solid var(--line); }
  .banner.alarm { display:block; background:#3a0c0c; color:#ffd0d0; border:1px solid var(--red); animation:flash .6s steps(1) infinite; }
  @keyframes flash { 50% { background:#5c1111; } }
  button { background:#0b2417; color:#d8ffe9; border:1px solid var(--line); border-radius:8px; padding:6px 12px; font:inherit; cursor:pointer; }
  button.on { border-color:var(--amber); color:var(--amber); }
</style>
</head>
<body>
<div class="wrap">
  <div class="card">
    <h1>Радар · HC-SR04 + SG90</h1>
    <canvas id="c"></canvas>
    <div class="banner" id="banner"></div>
    <div class="read">
      <span class="big" id="dist">—</span>
      <span class="muted" id="ang">кут —</span>
      <span class="muted" id="near">найближче: —</span>
    </div>
    <div class="ctl">
      <button id="guard">Увімкнути охорону</button>
      <label>Дальність <select id="range"><option>100</option><option selected>200</option><option>300</option></select> см</label>
      <label><input type="checkbox" id="mirror"> дзеркально</label>
      <span><span class="dot" id="dot"></span><span id="status">з'єднання…</span></span>
    </div>
  </div>
</div>
<script>
const cv = document.getElementById('c'), ctx = cv.getContext('2d');
const rangeEl = document.getElementById('range'), mirrorEl = document.getElementById('mirror');
const distEl = document.getElementById('dist'), angEl = document.getElementById('ang'), nearEl = document.getElementById('near');
const dot = document.getElementById('dot'), statusEl = document.getElementById('status');
const banner = document.getElementById('banner'), guardBtn = document.getElementById('guard');
guardBtn.onclick = async () => {
  try { await fetch('/api/guard?on=' + (st && st.g ? 0 : 1), { cache:'no-store' }); } catch (e) {}
  poll();
};
try { mirrorEl.checked = localStorage.getItem('mirror') === '1'; rangeEl.value = localStorage.getItem('range') || '200'; } catch (e) {}
mirrorEl.onchange = () => { try { localStorage.setItem('mirror', mirrorEl.checked ? '1' : '0'); } catch (e) {} draw(); };
rangeEl.onchange = () => { try { localStorage.setItem('range', rangeEl.value); } catch (e) {} draw(); };
let st = null, fails = 0;

function size() {
  const w = cv.clientWidth, dpr = window.devicePixelRatio || 1;
  cv.width = w * dpr; cv.height = (w / 2 + 28) * dpr; cv.style.height = (w / 2 + 28) + 'px';
  ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
}
function xy(cx, cy, r, deg) {          // 0° праворуч, 180° ліворуч, як у серво
  if (mirrorEl.checked) deg = 180 - deg;
  const a = deg * Math.PI / 180;
  return [cx + r * Math.cos(a), cy - r * Math.sin(a)];
}
function draw() {
  const w = cv.clientWidth, h = w / 2 + 28, cx = w / 2, cy = h - 18, R = w / 2 - 14;
  const range = +rangeEl.value;
  ctx.clearRect(0, 0, w, h);
  ctx.strokeStyle = 'rgba(61,255,143,.25)'; ctx.fillStyle = '#5f9a7a'; ctx.lineWidth = 1; ctx.font = '11px monospace';
  for (let k = 1; k <= 4; k++) {        // кільця дальності
    ctx.beginPath(); ctx.arc(cx, cy, R * k / 4, Math.PI, 2 * Math.PI); ctx.stroke();
    ctx.fillText(Math.round(range * k / 4) + ' см', cx + 4, cy - R * k / 4 + 12);
  }
  for (let d = 0; d <= 180; d += 30) {  // промені кутів
    const [x, y] = xy(cx, cy, R, d);
    ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(x, y); ctx.stroke();
    const [tx, ty] = xy(cx, cy, R + 2, d);
    ctx.fillText(d + '°', tx - 10, Math.min(ty - 2, cy + 14));
  }
  if (!st) return;
  // перешкоди
  const pts = st.d.map((cm, i) => ({ deg: st.min + i * st.step, cm }));
  ctx.fillStyle = 'rgba(61,255,143,.10)'; ctx.beginPath(); let open = false;
  for (const p of pts) {
    if (p.cm < 0 || p.cm > range) { if (open) { ctx.lineTo(cx, cy); ctx.fill(); ctx.beginPath(); open = false; } continue; }
    const [x, y] = xy(cx, cy, R * p.cm / range, p.deg);
    if (!open) { ctx.moveTo(cx, cy); open = true; }
    ctx.lineTo(x, y);
  }
  if (open) { ctx.lineTo(cx, cy); ctx.fill(); }
  for (const p of pts) {
    if (p.cm < 0 || p.cm > range) continue;
    const [x, y] = xy(cx, cy, R * p.cm / range, p.deg);
    ctx.fillStyle = p.cm < 30 ? '#ff5d5d' : p.cm < 100 ? '#ffc845' : '#3dff8f';
    ctx.beginPath(); ctx.arc(x, y, 3.5, 0, 2 * Math.PI); ctx.fill();
  }
  if (st.g) {   // охорона: «норма» кімнати — пунктир, нове — червоні кільця
    ctx.strokeStyle = 'rgba(200,220,210,.4)'; ctx.lineWidth = 1.5; ctx.setLineDash([4, 4]); ctx.beginPath();
    let on = false;
    st.b.forEach((cm, i) => {
      if (cm < 0 || cm > range) { on = false; return; }
      const [x, y] = xy(cx, cy, R * cm / range, st.min + i * st.step);
      if (on) ctx.lineTo(x, y); else ctx.moveTo(x, y);
      on = true;
    });
    ctx.stroke(); ctx.setLineDash([]);
    ctx.strokeStyle = '#ff5d5d'; ctx.lineWidth = 2;
    st.d.forEach((cm, i) => {
      if (st.c[i] !== '1' || cm < 0 || cm > range) return;
      const [x, y] = xy(cx, cy, R * cm / range, st.min + i * st.step);
      ctx.beginPath(); ctx.arc(x, y, 8, 0, 2 * Math.PI); ctx.stroke();
    });
  }
  // промінь
  const [bx, by] = xy(cx, cy, R, st.a);
  const g = ctx.createLinearGradient(cx, cy, bx, by);
  g.addColorStop(0, 'rgba(61,255,143,0)'); g.addColorStop(1, 'rgba(61,255,143,.9)');
  ctx.strokeStyle = g; ctx.lineWidth = 3; ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(bx, by); ctx.stroke();
}
async function poll() {
  try {
    const r = await fetch('/api/state', { cache:'no-store' });
    st = await r.json();
    distEl.textContent = st.cm === null ? '—' : Math.round(st.cm) + ' см';
    angEl.textContent = 'кут ' + st.a + '°';
    let best = null;
    st.d.forEach((cm, i) => { if (cm >= 0 && (!best || cm < best.cm)) best = { cm, deg: st.min + i * st.step }; });
    nearEl.textContent = best ? 'найближче: ' + best.cm + ' см на ' + best.deg + '°' : 'найближче: —';
    guardBtn.textContent = st.g ? 'Вимкнути охорону' : 'Увімкнути охорону';
    guardBtn.className = st.g ? 'on' : '';
    if (!st.g) banner.className = 'banner';
    else if (st.g === 1) { banner.className = 'banner learn'; banner.textContent = 'Запам\'ятовую кімнату… відійдіть від радара на кілька секунд'; }
    else if (st.al) { banner.className = 'banner alarm'; banner.textContent = 'ТРИВОГА: щось нове на ' + st.aa + '° (усього тривог: ' + st.n + ')'; }
    else { banner.className = 'banner calm'; banner.textContent = 'На варті: усе як було' + (st.n ? ' · тривог: ' + st.n : ''); }
    fails = 0; dot.className = 'dot online'; statusEl.textContent = 'на зв\'язку';
  } catch (e) {
    if (++fails > 2) { dot.className = 'dot'; statusEl.textContent = "немає зв'язку"; }
  }
  draw();
}
addEventListener('resize', () => { size(); draw(); });
size(); draw(); poll();
setInterval(poll, 120);
</script>
</body>
</html>
)rawliteral";

// ---------- Серво ----------
// SG90 чекає імпульс раз на 20 мс (50 Гц): 500 мкс = 0°, 2400 мкс = 180°.
void servoWrite(int deg) {
  int us = SERVO_MIN_US + (long)(SERVO_MAX_US - SERVO_MIN_US) * deg / 180;
  ledcWrite(SERVO_PIN, (uint32_t)us * 65535 / 20000);  // 16 біт на період 20000 мкс
}

int angleAt(int i) { return SWEEP_MIN + i * STEP_DEG; }

// ---------- Охорона ----------
void startLearning() {
  for (int i = 0; i < N; i++) {
    baseCm[i] = -1;
    changed[i] = false;
  }
  learnLeft = LEARN_PASSES + 1;  // +1: перший прохід почнеться з середини
  guard = G_LEARN;
  streak = 0;
  alarmUntil = 0;
  alarmAngle = -1;
  Serial.println("[guard] learning the room");
}

void stopGuard() {
  guard = G_OFF;
  alarmUntil = 0;
  for (int i = 0; i < N; i++) changed[i] = false;
  Serial.println("[guard] off");
}

// Чи стало на цьому куті щось ближче, ніж у «порожній кімнаті»
bool isCloser(int cm, int base) {
  if (cm < 0) return false;                // відлуння немає — нічого нового
  if (base < 0) return cm < GUARD_MAX_CM;  // було порожньо, а тепер щось є
  return cm < base - TOL_CM;
}

// ---------- Далекомір (як у парктроніку) ----------
float measureCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long us = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (us == 0) return -1;
  float cm = us / US_PER_CM;
  if (cm < MIN_VALID_CM) return -1;  // 1–2 см = наведення від власного передавача
  return cm;
}

// ---------- HTTP ----------
void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send_P(200, "text/html; charset=utf-8", PAGE);
}

void handleState() {
  String json;
  json.reserve(120 + N * 11);
  json += "{\"a\":";
  json += angleAt(idx);
  json += ",\"cm\":";
  json += lastCm < 0 ? String("null") : String(lastCm, 1);
  json += ",\"min\":";
  json += SWEEP_MIN;
  json += ",\"step\":";
  json += STEP_DEG;
  json += ",\"d\":[";
  for (int i = 0; i < N; i++) {
    if (i) json += ',';
    json += scanCm[i];
  }
  json += "],\"g\":";
  json += (int)guard;  // 0 вимкнено, 1 вчиться, 2 на варті
  json += ",\"al\":";
  json += millis() < alarmUntil ? "true" : "false";
  json += ",\"aa\":";
  json += alarmAngle;
  json += ",\"n\":";
  json += alarms;
  json += ",\"b\":[";
  for (int i = 0; i < N; i++) {
    if (i) json += ',';
    json += baseCm[i];
  }
  json += "],\"c\":\"";
  for (int i = 0; i < N; i++) json += changed[i] ? '1' : '0';
  json += "\"}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleGuard() {
  if (server.arg("on") == "1") startLearning();  // увімкнення завжди заново вчить кімнату
  else stopGuard();
  handleState();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("=== ESP32 radar ===");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < N; i++) {
    scanCm[i] = -1;
    baseCm[i] = -1;
    changed[i] = false;
  }

  ledcAttach(SERVO_PIN, 50, 16);  // 50 Гц, 16 біт
  servoWrite(angleAt(idx));

  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP '%s' %s, IP: %s\n", AP_SSID, ok ? "started" : "FAILED",
                WiFi.softAPIP().toString().c_str());

  server.on("/", handleRoot);
  server.on("/api/state", handleState);
  server.on("/api/guard", handleGuard);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.println("HTTP server started on port 80");
  lastStep = millis();
}

void loop() {
  server.handleClient();
  unsigned long now = millis();
  if (now - lastStep < STEP_MS) return;
  lastStep = now;

  // 1. Серво вже STEP_MS стоїть на куті idx — міряємо
  lastCm = measureCm();
  int cm = lastCm < 0 ? -1 : (int)(lastCm + 0.5f);
  scanCm[idx] = cm;

  // 2. Охорона: вчимо «норму» або порівнюємо з нею
  if (guard == G_LEARN) {
    if (cm >= 0 && (baseCm[idx] < 0 || cm < baseCm[idx])) baseCm[idx] = cm;  // норма = найближче з проходів
  } else if (guard == G_ARMED) {
    changed[idx] = isCloser(cm, baseCm[idx]);
    streak = changed[idx] ? streak + 1 : 0;
    // тривога лише коли «нове» на кількох кутах підряд: одиночні стрибки датчика не рахуються
    if (streak >= CONFIRM_STEPS) {
      if (now >= alarmUntil) {
        alarms++;
        Serial.printf("[guard] ALARM #%lu at %d deg: %d cm (norm %d)\n", (unsigned long)alarms,
                      angleAt(idx), cm, baseCm[idx]);
      }
      alarmUntil = now + ALARM_HOLD_MS;
      alarmAngle = angleAt(idx);
    }
  }

  // 3. KY-034: вчиться — повільно блимає; на варті — світить при тривозі; інакше — ближче ALERT_CM
  bool led;
  if (guard == G_LEARN) led = (now / 500) % 2;
  else if (guard == G_ARMED) led = now < alarmUntil;
  else led = cm >= 0 && cm < ALERT_CM;
  digitalWrite(LED_PIN, led ? HIGH : LOW);

  // 4. Наступний кут; на краю розвертаємось
  if (idx + dir < 0 || idx + dir >= N) {
    dir = -dir;
    sweeps++;
    if (guard == G_LEARN && --learnLeft == 0) {
      guard = G_ARMED;
      Serial.println("[guard] armed");
    }
    int best = -1;
    for (int i = 0; i < N; i++)
      if (scanCm[i] >= 0 && (best < 0 || scanCm[i] < scanCm[best])) best = i;
    if (best < 0) Serial.printf("sweep %lu: nothing\n", (unsigned long)sweeps);
    else Serial.printf("sweep %lu: nearest %d cm at %d deg\n", (unsigned long)sweeps, scanCm[best], angleAt(best));
  }
  idx += dir;
  servoWrite(angleAt(idx));
}
