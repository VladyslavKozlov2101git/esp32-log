# esp32-log

Журнал навчання ESP32: що зроблено, які були проблеми, як їх вирішили.

- [`LOG.md`](LOG.md) — журнал, нові сесії додаються вгору.
- [`hardware/INVENTORY.md`](hardware/INVENTORY.md) — інвентар обладнання (що є, у якому стані, що з нього можна зробити).
- [`lessons/`](lessons/) — уроки з рисунками схем: [00 — піни ESP32 і TTP223B](lessons/00-esp32-pins/README.md), [01 — сенсорна кнопка TTP223B](lessons/01-touch-counter/README.md), [02 — Морзе-ключ зі світлодіодом](lessons/02-morse-led/README.md), [03 — парктронік](lessons/03-parktronic/README.md), [04 — ультразвуковий радар](lessons/04-radar/README.md).
- [`docs/breadboard.md`](docs/breadboard.md) — як користуватись макетною платою і як під'єднати перший модуль.
- `hardware/photos/` — фото компонентів для ідентифікації.

## Проєкти

| Проєкт | Що робить | Технології |
| --- | --- | --- |
| [`projects/esp32-blink`](projects/esp32-blink) | блимає LED, пише в Serial | C++ (Arduino) |
| [`projects/esp32-touch-counter`](projects/esp32-touch-counter) | сенсор TTP223B + вебсторінка-лічильник | C++ (Arduino), HTML, CSS, JS |
| [`projects/esp32-morse`](projects/esp32-morse) | сенсор як телеграфний ключ: крапки/тире → текст на сторінці | C++ (Arduino), HTML, CSS, JS |
| [`projects/esp32-probe`](projects/esp32-probe) | щуп-вольтметр: читає 6 входів ADC, показує min/max під час імпульсу | C++ (Arduino) |
| [`projects/esp32-servo-test`](projects/esp32-servo-test) | перевірка серво: розгортка 0–180° без датчиків | C++ (Arduino) |
| [`projects/esp32-radar`](projects/esp32-radar) | радар: HC-SR04 + серво + KY-034, вебсторінка з розгорткою | C++ (Arduino), HTML, CSS, JS |
| [`projects/esp32-parktronic`](projects/esp32-parktronic) | HC-SR04 міряє відстань, KY-034 блимає частіше ближче до перешкоди, сантиметри на сторінці | C++ (Arduino), HTML, CSS, JS |
| [`projects/esp32-radar`](projects/esp32-radar) | HC-SR04 на сервоприводі SG90 оглядає 150°, телефон малює екран радара | C++ (Arduino), HTML, CSS, JS (canvas) |

Кожен проєкт збирається через PlatformIO:

```bash
cd projects/esp32-touch-counter
pio run -t upload
pio device monitor
```

## Як додати запис

```bash
cd ~/Desktop/esp32-log
# відредагувати LOG.md
git add .
git commit -m "Сесія N: короткий опис"
```

## Пов'язані проєкти

| Проєкт | Шлях |
| --- | --- |
| blink (перша прошивка) | `C:\esp32\esp32-blink` |
| touch-counter (сенсор + вебсторінка) | `C:\esp32\esp32-touch-counter` |
