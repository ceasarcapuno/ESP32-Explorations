# Analog Input with Potentiometer and PWM

Uses a potentiometer to control LED brightness in real time on the ESP32-S3. The ADC reading from the potentiometer wiper is mapped to an 8-bit PWM duty cycle via the Arduino-ESP32 LEDC API.

## Hardware

| Component | Pin | Notes |
|-----------|-----|-------|
| Potentiometer (wiper) | GPIO 4 | ADC input |
| Potentiometer (end 1) | 3V3 | High reference |
| Potentiometer (end 2) | GND | Low reference |
| LED (anode) | GPIO 7 | Active HIGH — pin → LED → 330 Ω → GND |

See [`resource/circuit diagram.PNG`](resource/circuit%20diagram.PNG) for the wiring diagram.

## Output

```
=== Potentiometer → PWM LED Brightness ===
Raw (0-4095) | Voltage (V) | Duty (0-255)
------------------------------------------
0            | 0.00 V      | 0
2048         | 1.65 V      | 128
4095         | 3.30 V      | 255
```

## Software

- **Platform:** ESP32-S3 DevKitC-1
- **Framework:** Arduino (arduino-esp32 v2.x)
- **Build system:** PlatformIO
- **PWM:** LEDC channel 0, 5 kHz, 8-bit resolution

## Getting Started

1. Open the `Analog-Input-with-Potentiometer-and-PWM` folder in PlatformIO.
2. Wire the components as described above.
3. Build and upload:
   ```
   pio run --target upload
   ```
4. Open the serial monitor at **115200 baud** and turn the potentiometer — the LED brightness will follow.

## License

MIT — see [LICENSE](../LICENSE).
