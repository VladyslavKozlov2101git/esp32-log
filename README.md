# esp32-log

Журнал навчання ESP32: що зроблено, які були проблеми, як їх вирішили.

- [`LOG.md`](LOG.md) — журнал, нові сесії додаються вгору.
- [`hardware/INVENTORY.md`](hardware/INVENTORY.md) — інвентар обладнання (що є, у якому стані, що з нього можна зробити).
- [`lessons/`](lessons/) — уроки з рисунками схем: [00 — піни ESP32 і TTP223B](lessons/00-esp32-pins/README.md), [01 — сенсорна кнопка TTP223B](lessons/01-touch-counter/README.md).
- [`docs/breadboard.md`](docs/breadboard.md) — як користуватись макетною платою і як під'єднати перший модуль.
- `hardware/photos/` — фото компонентів для ідентифікації.

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
