#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// ---------- Налаштування ----------
static const int TOUCH_PIN = 4;  // TTP223B OUT -> GPIO4

static const char *AP_SSID = "ESP32-Morse";
static const char *AP_PASS = "12345678";

// Тайминги (можна підлаштувати під себе)
static const unsigned long DOT_MAX_MS = 300;     // коротше — крапка, довше — тире
static const unsigned long LETTER_GAP_MS = 700;  // пауза = кінець літери
static const unsigned long WORD_GAP_MS = 2000;   // довша пауза = пробіл
static const size_t MAX_TEXT = 160;

// ---------- Таблиця Морзе ----------
struct MorseCode {
  const char *code;
  char ch;
};

static const MorseCode MORSE[] = {
    {".-", 'A'},   {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'},  {".", 'E'},
    {"..-.", 'F'}, {"--.", 'G'},  {"....", 'H'}, {"..", 'I'},   {".---", 'J'},
    {"-.-", 'K'},  {".-..", 'L'}, {"--", 'M'},   {"-.", 'N'},   {"---", 'O'},
    {".--.", 'P'}, {"--.-", 'Q'}, {".-.", 'R'},  {"...", 'S'},  {"-", 'T'},
    {"..-", 'U'},  {"...-", 'V'}, {".--", 'W'},  {"-..-", 'X'}, {"-.--", 'Y'},
    {"--..", 'Z'}, {"-----", '0'}, {".----", '1'}, {"..---", '2'}, {"...--", '3'},
    {"....-", '4'}, {".....", '5'}, {"-....", '6'}, {"--...", '7'}, {"---..", '8'},
    {"----.", '9'}, {".-.-.-", '.'}, {"--..--", ','}, {"..--..", '?'}, {"-..-.", '/'},
};
static const size_t MORSE_COUNT = sizeof(MORSE) / sizeof(MORSE[0]);

char decodeMorse(const String &code) {
  for (size_t i = 0; i < MORSE_COUNT; i++) {
    if (code == MORSE[i].code) return MORSE[i].ch;
  }
  return '?';
}

// ---------- Стан ----------
static String buffer;       // поточна літера: ".-"
static String text;         // розшифрований текст
static String lastSymbol;   // останній символ: "." або "-"
static bool pressed = false;
static bool hasRelease = false;
static unsigned long pressStart = 0;
static unsigned long releaseAt = 0;
static uint32_t symbolCount = 0;

WebServer server(80);

// ---------- Сторінка ----------
static const char PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="uk">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Морзе · ESP32</title>
<style>
  :root { --bg:#0b0f18; --card:#141a29; --line:#232c42; --accent:#7dd3fc; --muted:#8892ab; }
  * { box-sizing:border-box; -webkit-tap-highlight-color:transparent; }
  body {
    margin:0; min-height:100vh; padding:20px; display:flex; justify-content:center;
    background:radial-gradient(900px 500px at 50% -15%, #18233b 0%, var(--bg) 65%);
    color:#e8eefc; font-family:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  }
  .wrap { width:min(520px,100%); }
  .card { background:var(--card); border:1px solid var(--line); border-radius:20px; padding:24px; box-shadow:0 18px 50px rgba(0,0,0,.45); }
  h1 { font-size:13px; letter-spacing:.16em; text-transform:uppercase; color:var(--muted); margin:0 0 16px; font-weight:700; }
  .text {
    min-height:96px; font-family:ui-monospace,SFMono-Regular,Consolas,monospace; font-size:22px;
    line-height:1.5; letter-spacing:.06em; word-break:break-word; color:#e8eefc;
  }
  .cursor { display:inline-block; width:10px; height:22px; background:var(--accent); vertical-align:-3px; animation:blink 1s steps(1) infinite; }
  @keyframes blink { 50% { opacity:0; } }
  .buffer { margin-top:10px; font-family:ui-monospace,Consolas,monospace; font-size:28px; color:var(--accent); min-height:34px; letter-spacing:.22em; }
  .row { display:flex; align-items:center; gap:10px; margin-top:14px; font-size:13px; color:var(--muted); }
  .dot { width:9px; height:9px; border-radius:50%; background:#f59e0b; }
  .dot.online { background:#34d399; box-shadow:0 0 10px #34d399; }
  .dot.touch { background:var(--accent); box-shadow:0 0 14px var(--accent); }
  button {
    margin-top:18px; width:100%; padding:13px; border-radius:12px; cursor:pointer;
    border:1px solid rgba(255,255,255,.12); background:rgba(255,255,255,.05); color:#e8eefc; font-size:15px; font-weight:600;
  }
  button:active { background:rgba(255,255,255,.12); }
  .hint { font-size:13px; color:var(--muted); line-height:1.6; margin:14px 0 0; }
  .kbd { display:grid; grid-template-columns:repeat(auto-fill,minmax(76px,1fr)); gap:6px; margin-top:18px; }
  .kbd div { font-family:ui-monospace,Consolas,monospace; font-size:12px; color:var(--muted); background:rgba(255,255,255,.03); border:1px solid var(--line); border-radius:8px; padding:5px 7px; }
  .kbd b { color:#cbd5e1; font-weight:700; margin-right:4px; }
</style>
</head>
<body>
<div class="wrap">
  <div class="card">
    <h1>Телеграф · TTP223B</h1>
    <div class="text" id="text"><span class="cursor"></span></div>
    <div class="buffer" id="buffer"></div>
    <div class="row"><span class="dot" id="dot"></span><span id="status">з'єднання…</span></div>
    <button id="clear">Очистити</button>
    <p class="hint">
      Короткий дотик — <b>крапка</b>, довший (&gt; 0.3 с) — <b>тире</b>.<br>
      Пауза 0.7 с завершує літеру, 2 с — додає пробіл.
    </p>
    <div class="kbd" id="kbd"></div>
  </div>
</div>
<script>
const textEl = document.getElementById('text');
const bufEl = document.getElementById('buffer');
const dot = document.getElementById('dot');
const statusEl = document.getElementById('status');
let fails = 0, lastText = null, lastBuf = null;

function esc(s) {
  return s.replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
}

async function poll() {
  try {
    const r = await fetch('/api/state', { cache:'no-store' });
    const d = await r.json();
    if (d.text !== lastText) {
      lastText = d.text;
      textEl.innerHTML = esc(d.text) + '<span class="cursor"></span>';
      if (d.text) textEl.scrollTop = textEl.scrollHeight;
    }
    if (d.buffer !== lastBuf) { lastBuf = d.buffer; bufEl.textContent = d.buffer; }
    fails = 0;
    dot.className = 'dot online' + (d.touch ? ' touch' : '');
    statusEl.textContent = d.touch ? 'дотик — тримайте для тире' : 'готово';
  } catch (e) {
    if (++fails > 2) { dot.className = 'dot'; statusEl.textContent = "немає зв'язку"; }
  }
}

document.getElementById('clear').onclick = async () => {
  try { await fetch('/api/clear', { cache:'no-store' }); } catch(e) {}
  lastText = null; lastBuf = null; poll();
};

const KEYBOARD = [['A','.-'],['B','-...'],['C','-.-.'],['D','-..'],['E','.'],['F','..-.'],
  ['G','--.'],['H','....'],['I','..'],['J','.---'],['K','-.-'],['L','.-..'],['M','--'],
  ['N','-.'],['O','---'],['P','.--.'],['Q','--.-'],['R','.-.'],['S','...'],['T','-'],
  ['U','..-'],['V','...-'],['W','.--'],['X','-..-'],['Y','-.--'],['Z','--..'],
  ['0','-----'],['1','.----'],['2','..---'],['3','...--'],['4','....-'],
  ['5','.....'],['6','-....'],['7','--...'],['8','---..'],['9','----.']];
document.getElementById('kbd').innerHTML =
  KEYBOARD.map(([c,m]) => '<div><b>' + c + '</b>' + m + '</div>').join('');

poll();
setInterval(poll, 200);
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
  String json = "{\"text\":\"";
  json += text;
  json += "\",\"buffer\":\"";
  json += buffer;
  json += "\",\"touch\":";
  json += pressed ? "true" : "false";
  json += "}";
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

void handleClear() {
  text = "";
  buffer = "";
  lastSymbol = "";
  hasRelease = false;
  Serial.println("[morse] cleared");
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", "{\"ok\":true}");
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("=== ESP32 morse key ===");

  pinMode(TOUCH_PIN, INPUT);

  bool ok = WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP '%s' %s, IP: %s\n", AP_SSID, ok ? "started" : "FAILED",
                WiFi.softAPIP().toString().c_str());

  server.on("/", handleRoot);
  server.on("/api/state", handleState);
  server.on("/api/clear", handleClear);
  server.onNotFound([]() { server.send(404, "text/plain", "Not found"); });
  server.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  server.handleClient();

  bool raw = digitalRead(TOUCH_PIN) == HIGH;
  unsigned long now = millis();

  // Початок дотику
  if (raw && !pressed) {
    pressed = true;
    pressStart = now;
  }

  // Кінець дотику — визначаємо крапку/тире
  if (!raw && pressed) {
    pressed = false;
    unsigned long held = now - pressStart;
    lastSymbol = (held < DOT_MAX_MS) ? "." : "-";
    buffer += lastSymbol;
    symbolCount++;
    releaseAt = now;
    hasRelease = true;
    Serial.printf("held %lu ms -> %s   [%s]\n", held, lastSymbol.c_str(), buffer.c_str());
  }

  // Пауза: завершити літеру або додати пробіл
  if (!pressed && hasRelease) {
    unsigned long gap = now - releaseAt;

    if (buffer.length() > 0 && gap > LETTER_GAP_MS) {
      char ch = decodeMorse(buffer);
      if (text.length() < MAX_TEXT) text += ch;
      Serial.printf("letter '%s' -> '%c'   text: %s\n", buffer.c_str(), ch, text.c_str());
      buffer = "";
      lastSymbol = "";
    } else if (buffer.length() == 0 && gap > WORD_GAP_MS && text.length() > 0 &&
               !text.endsWith(" ")) {
      if (text.length() < MAX_TEXT) text += ' ';
      Serial.println("[morse] space");
    }
  }
}
