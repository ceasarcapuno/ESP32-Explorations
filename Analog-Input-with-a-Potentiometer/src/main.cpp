/*
 * ============================================================================
 * Project:     ESP32 Analog Input with a Potentiometer
 * File:        main.cpp
 * Author:      Ceasar Capuno
 * Created:     2026
 * Version:     1.0.0
 *
 * Description:
 *   Reads an analog voltage from a potentiometer wiper connected to GPIO 4
 *   using the ESP32-S3 12-bit ADC. Raw ADC counts (0–4095) and the mapped
 *   voltage (0.00–3.30 V) are printed to the Serial monitor at 115200 baud,
 *   once every 200 ms.
 *
 * License:
 *   This source code is released into the public domain under the terms
 *   of the MIT License. You are free to use, copy, modify, merge,
 *   publish, distribute, sublicense, and/or sell copies of this software,
 *   in whole or in part, with or without modification, for any purpose,
 *   commercial or non-commercial, without restriction.
 *
 *   The only requirement is that the original author attribution above
 *   is retained in all copies or substantial portions of the software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 *   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Hardware:
 *   - MCU:          ESP32-S3 (any variant)
 *   - Potentiometer: GPIO 4 (wiper / signal pin)
 *                    3V3  → left pin
 *                    GND  → right pin
 *
 * Repository:  https://github.com/ceasarcapuno/ESP32-Explorations
 * Contact:     ceasar.capuno@gmail.com 
 * ============================================================================
 */

#include <Arduino.h>

// ── Pin & ADC configuration ──────────────────────────────────────────────────
static constexpr int   POT_PIN       = 4;       // ADC input — potentiometer wiper
static constexpr int   ADC_BITS      = 12;      // ESP32-S3 ADC resolution
static constexpr int   ADC_MAX       = (1 << ADC_BITS) - 1;  // 4095
static constexpr float VREF          = 3.3f;    // Reference voltage (V)
static constexpr int   SAMPLE_PERIOD = 200;     // Serial print interval (ms)

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    analogReadResolution(ADC_BITS);
    Serial.println("=== Potentiometer ADC Reader ===");
    Serial.println("Raw (0-4095) | Voltage (V)");
    Serial.println("-------------------------------");
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    int   raw     = analogRead(POT_PIN);
    float voltage = (raw / (float)ADC_MAX) * VREF;

    Serial.printf("%-13d| %.2f V\n", raw, voltage);
    delay(SAMPLE_PERIOD);
}
