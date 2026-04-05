/*
 * ============================================================================
 * Project:     ESP32 Analog Input with Potentiometer and PWM
 * File:        main.cpp
 * Author:      Ceasar Capuno
 * Created:     2026
 * Version:     1.0.0
 *
 * Description:
 *   Reads a potentiometer wiper on GPIO 4 using the ESP32-S3's 12-bit ADC,
 *   then maps the result to an 8-bit PWM duty cycle that controls the
 *   brightness of an LED on GPIO 7 via the Arduino-ESP32 v2.x LEDC API.
 *   Turning the potentiometer from GND to 3V3 smoothly fades the LED from
 *   fully off to fully on. Raw ADC count, voltage, and duty cycle are
 *   printed to the Serial monitor every 100 ms.
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
 *   - LED:           GPIO 7, active HIGH (pin → LED → 330 Ω → GND)
 *
 * Repository:  https://github.com/ceasarcapuno/ESP32-Explorations
 * Contact:     ceasar.capuno@gmail.com
 * ============================================================================
 */

#include <Arduino.h>

// ── Pin & peripheral configuration ───────────────────────────────────────────
static constexpr int   POT_PIN      = 4;    // ADC input  — potentiometer wiper
static constexpr int   LED_PIN      = 7;    // PWM output — LED anode

static constexpr int   ADC_BITS     = 12;   // ESP32-S3 ADC resolution
static constexpr int   ADC_MAX      = (1 << ADC_BITS) - 1;  // 4095
static constexpr float VREF         = 3.3f;

static constexpr int   PWM_CHANNEL  = 0;
static constexpr int   PWM_FREQ     = 5000; // Hz
static constexpr int   PWM_BITS     = 8;    // 8-bit duty → 0–255
static constexpr int   PWM_MAX      = (1 << PWM_BITS) - 1;  // 255

static constexpr int   SAMPLE_PERIOD = 100; // Serial print interval (ms)

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    analogReadResolution(ADC_BITS);

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_BITS);
    ledcAttachPin(LED_PIN, PWM_CHANNEL);

    Serial.println("=== Potentiometer → PWM LED Brightness ===");
    Serial.println("Raw (0-4095) | Voltage (V) | Duty (0-255)");
    Serial.println("------------------------------------------");
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    int   raw     = analogRead(POT_PIN);
    float voltage = (raw / (float)ADC_MAX) * VREF;
    int   duty    = map(raw, 0, ADC_MAX, 0, PWM_MAX);

    ledcWrite(PWM_CHANNEL, duty);

    Serial.printf("%-13d| %.2f V      | %d\n", raw, voltage, duty);
    delay(SAMPLE_PERIOD);
}
