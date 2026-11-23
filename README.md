# esp32-log

**English** · [Українська](README.uk.md)

A hobby log of learning ESP32 from scratch: small hardware projects, what broke and how I fixed it.

Most projects pair a sensor with a web page. The ESP32 starts its own Wi-Fi access point and serves the page, and the page polls the board over HTTP. No cloud and no app, just a phone browser.

**Highlight — ultrasonic radar.** An HC-SR04 sensor on an SG90 servo sweeps 150°, and the phone draws a radar screen on a canvas in real time. Code: [`projects/esp32-radar`](projects/esp32-radar), wiring and notes: [lesson 04](lessons/04-radar/README.md).

## Projects

| Project | What it does | Tech |
| --- | --- | --- |
| [`esp32-blink`](projects/esp32-blink) | Blinks the LED and logs to Serial (first firmware) | C++ (Arduino) |
| [`esp32-touch-counter`](projects/esp32-touch-counter) | TTP223B touch sensor + a web page that counts taps | C++, HTML, CSS, JS |
| [`esp32-morse`](projects/esp32-morse) | Touch sensor as a telegraph key: dots and dashes become text on the page | C++, HTML, CSS, JS |
| [`esp32-probe`](projects/esp32-probe) | Probe voltmeter: reads 6 ADC inputs and shows min/max during a pulse | C++ |
| [`esp32-servo-test`](projects/esp32-servo-test) | Servo check: 0–180° sweep without sensors | C++ |
| [`esp32-parktronic`](projects/esp32-parktronic) | Parking sensor: HC-SR04 measures distance, an LED blinks faster as you get closer, centimetres on the page | C++, HTML, CSS, JS |
| [`esp32-radar`](projects/esp32-radar) | Ultrasonic radar: HC-SR04 on a servo, radar screen on the phone | C++, HTML, CSS, JS (canvas) |

Hardware: ESP32 DevKit (esp32dev), HC-SR04, SG90, TTP223B, KY-034, breadboard. Full list in [`hardware/INVENTORY.md`](hardware/INVENTORY.md).

## Build

Each project builds with [PlatformIO](https://platformio.org/):

```bash
cd projects/esp32-radar
pio run -t upload
pio device monitor
```

## Notes and lessons (in Ukrainian)

- [`LOG.md`](LOG.md) — session journal, newest first: what I tried, what failed and why (for example, a servo browning out the board on USB power until it got its own 5 V supply).
- [`lessons/`](lessons/) — step-by-step lessons with wiring diagrams: [00 — ESP32 pins](lessons/00-esp32-pins/README.md), [01 — touch button](lessons/01-touch-counter/README.md), [02 — Morse key](lessons/02-morse-led/README.md), [03 — parking sensor](lessons/03-parktronic/README.md), [04 — radar](lessons/04-radar/README.md).
- [`docs/breadboard.md`](docs/breadboard.md) — how a breadboard works and how to connect the first module.
