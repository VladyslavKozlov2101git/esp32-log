# Журнал навчання ESP32

Записи ведуться від найновішого до найстарішого всередині кожного розділу.

---

## 2026-11-13 — Сесія 1: середовище та перша прошивка

### Залізо

| Параметр | Значення |
| --- | --- |
| ПК | HP ProBook 4740s, Windows 10 Pro for Workstations (10.0.19045) |
| Плата | ESP32 DevKit (ESP32-D0WD-V3), 2 ядра, 4 МБ flash |
| USB-міст | CH340 (VID_1A86, PID_7523) |
| Порт | `COM3` |

### Встановлено

- **VS Code** 1.124.2 (уже був)
- Розширення **PlatformIO IDE** 3.3.4 + **C/C++** (ms-vscode.cpptools) 1.35.6
- **PlatformIO Core** 6.2.0 (`pio` у PATH)
- Платформа **espressif32** + Xtensa-тулчейн (у `C:\Users\vladi\.platformio`)

### Створено проєкт

`C:\esp32\esp32-blink`

```
esp32-blink/
├── platformio.ini     # env:esp32dev, framework=arduino, monitor 115200
├── src/main.cpp       # blink на GPIO2 + вивід у Serial
└── README.md          # шпаргалка + план навчання
```

### Результат

Прошивка зібралась і залилась, Serial працює:

```
=== ESP32 blink started ===
Chip: ESP32-D0WD-V3 | cores: 2 | flash: 4096 KB
Free heap: 331796 bytes
LED ON
LED OFF
...
```

RAM 6.7% (22 116 B з 327 680), Flash 22.2% (291 612 B з 1 310 720).

### Проблеми та рішення

1. **Кирилиця в шляху ламає лінкування.**
   Проєкт лежав у `Documents\Типовий проєкт` — лінкер падав:
   `cannot open map file .../firmware.map: No such file or directory`
   (шлях у виводі перетворювався на `�������`).
   → Рішення: тримати ESP32-проєкти на **ASCII-шляху без пробілів і OneDrive**.
   Проєкт перенесено в `C:\esp32\esp32-blink`.

2. **Плата не визначалась (не було COM-порту).**
   Windows не бачив жодного USB-пристрою, навіть «невідомого» — тобто драйвер ні до чого.
   Причина: кабель «тільки зарядка» (без ліній D+/D−) або порт.
   → Рішення: кабель з підтримкою передачі даних + інший USB-порт.
   Після цього з'явився `USB-SERIAL CH340 (COM3)` — драйвер CH340 уже був у системі.

### Корисні команди

```bash
cd C:\esp32\esp32-blink
pio run                  # зібрати
pio run -t upload        # прошити
pio device monitor       # Serial-монітор, вихід — Ctrl+C
pio device list          # список COM-портів
```

### Наступні кроки

- [ ] Кнопка + LED: `digitalRead`, внутрішній pull-up, антидребезг
- [ ] PWM (LEDC): плавне регулювання яскравості
- [ ] АЦП + потенціометр: `analogRead`, шум АЦП
- [ ] I2C-датчик (BME280/DHT22)
- [ ] Wi-Fi: точка доступу + HTTP-сторінка
- [ ] FreeRTOS: задачі, черги
- [ ] Deep sleep / живлення від батареї
