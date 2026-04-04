# LED Effects Controller

Button-driven multi-mode LED controller for the ESP32-S3, built with the Arduino-ESP32 v2.x LEDC (PWM) API.

## Modes

Each button press cycles through the following LED effects, then turns the LED off:

| # | Mode | Behaviour | Exit |
|---|------|-----------|------|
| 1 | **Steady ON** | LED on at full brightness | Single press |
| 2 | **Blink** | Toggles every 300 ms | Single press |
| 3 | **Breathing** | Linear PWM fade 0 → 255, 3 s cycle | Single press |
| 4 | **Morse** | Spells **C E A S A R** in Morse code, then holds steady 2 s and repeats | Double-press within 1 s |

## Hardware

| Component | Pin | Notes |
|-----------|-----|-------|
| Button | GPIO 4 | Active HIGH — 3V3 → button → pin, 4.7 kΩ pull-down to GND |
| LED | GPIO 7 | Active HIGH — pin → 330 Ω → LED → GND |

## Software

- **Platform:** ESP32-S3 DevKitC-1
- **Framework:** Arduino (arduino-esp32 v2.x)
- **Build system:** PlatformIO

## Getting Started

1. Clone the repo and open the `LED-Effects-Controller` folder in PlatformIO.
2. Wire the button and LED as described above.
3. Build and upload:
   ```
   pio run --target upload
   ```
4. Open the serial monitor at **115200 baud** to watch state transitions.

## License

MIT — see [LICENSE](../LICENSE).
