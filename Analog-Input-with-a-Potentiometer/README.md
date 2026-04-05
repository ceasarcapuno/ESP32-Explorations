# Analog Input with a Potentiometer

Reads a potentiometer via the ESP32-S3's 12-bit ADC and streams the raw count and mapped voltage to the Serial monitor.

## Hardware

| Component | Pin | Notes |
|-----------|-----|-------|
| Potentiometer (wiper) | GPIO 4 | ADC input |
| Potentiometer (end 1) | 3V3 | High reference |
| Potentiometer (end 2) | GND | Low reference |

See [`resource/circuit diagram.PNG`](resource/circuit%20diagram.PNG) for the wiring diagram.

## Output

```
=== Potentiometer ADC Reader ===
Raw (0-4095) | Voltage (V)
-------------------------------
2048         | 1.65 V
3100         | 2.50 V
...
```

- **Raw** — 12-bit ADC count (0 – 4095)
- **Voltage** — linearly mapped to 0.00 – 3.30 V

## Software

- **Platform:** ESP32-S3 DevKitC-1
- **Framework:** Arduino (arduino-esp32 v2.x)
- **Build system:** PlatformIO

## Getting Started

1. Open the `Analog-Input-with-a-Potentiometer` folder in PlatformIO.
2. Wire the potentiometer as described above.
3. Build and upload:
   ```
   pio run --target upload
   ```
4. Open the serial monitor at **115200 baud** and turn the potentiometer knob to see the values change.

## License

MIT — see [LICENSE](../LICENSE).
