/*
 * ============================================================================
 * Project:     ESP32 LED Effects Controller
 * File:        main.cpp
 * Author:      Ceasar Capuno
 * Created:     2026
 * Version:     1.0.0
 *
 * Description:
 *   Button-driven multi-mode LED controller for ESP32. Cycles through four
 *   effects on each button press — Steady ON, Blink (300 ms), Breathing
 *   (PWM fade, 3 s cycle), and Morse code spelling "CEASAR" — using the
 *   Arduino-ESP32 v2.x LEDC API with a 4-state software debounce and
 *   double-press detection for exiting Morse mode.
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
 *   - MCU:    ESP32 (any variant)
 *   - Button: GPIO 4, active HIGH (3V3 → button → pin, 4.7kΩ pull-down)
 *   - LED:    GPIO 7, active HIGH (pin → 330Ω → LED → GND)
 *
 * Repository:  https://github.com/ceasarcapuno/ESP32-Explorations
 * Contact:     ceasar.capuno@gmail.com
 * ============================================================================
 */

#include <Arduino.h>

// ── Pin definitions ───────────────────────────────────────────────────────────
#define BUTTON_PIN  4   // Active HIGH (3V3 → button → pin, 4.7kΩ pull-down to GND)
#define LED_PIN     7   // Active HIGH (pin → 330Ω → LED → GND)

// ── PWM (LEDC) – Arduino-ESP32 v2.x API ──────────────────────────────────────
#define PWM_CHANNEL     0
#define PWM_FREQ        5000
#define PWM_RESOLUTION  8     // 8-bit: duty 0–255

// ── State machine ─────────────────────────────────────────────────────────────
enum State { STATE_OFF, STATE_STEADY, STATE_BLINK, STATE_BREATHING, STATE_MORSE };

State state    = STATE_OFF;
int   nextMode = 0;   // 0=STEADY, 1=BLINK, 2=BREATHING, 3=MORSE (cycles 0→1→2→3→0)

// ── Button debounce & press detection ────────────────────────────────────────
// 4-state machine: IDLE → PRESSING (wait 80ms) → HELD → RELEASING (wait 80ms) → IDLE
// A new press can only register AFTER a full confirmed release.
enum BtnState { BTN_IDLE, BTN_PRESSING, BTN_HELD, BTN_RELEASING };
BtnState      btnState     = BTN_IDLE;
unsigned long btnTimer     = 0;
bool          buttonPressed = false;
const unsigned long DEBOUNCE_MS = 80;

// Double-press detection (used to exit Morse mode)
unsigned long lastPressTime = 0;
int           pressCount    = 0;
bool          doublePressed = false;
const unsigned long DOUBLE_PRESS_WINDOW = 1000;

// ── Blink ─────────────────────────────────────────────────────────────────────
bool          blinkState    = false;
unsigned long blinkTimer    = 0;
const unsigned long BLINK_INTERVAL = 300;

// ── Breathing ─────────────────────────────────────────────────────────────────
unsigned long breathStart = 0;
const unsigned long BREATH_CYCLE = 3000;

// ── Morse code ────────────────────────────────────────────────────────────────
// Spell: C E A S A R
// C: −·−·   E: ·   A: ·−   S: ···   A: ·−   R: ·−·
// Timings (ms)
#define DOT_MS       120   // 1 unit ON
#define DASH_MS      360   // 3 units ON
#define INTRA_MS     120   // 1 unit OFF  (between elements of a letter)
#define INTERCHAR_MS 360   // 3 units OFF (between letters)
#define STEADY_MS   2000   // pause with steady light after each full word

struct Seg { uint16_t dur; bool on; };

static Seg  morse[64];
static int  morseLen = 0;
static int  morseIdx = 0;
static unsigned long morseSegStart  = 0;
static bool morseSteady = false;
static unsigned long morseSteadyStart = 0;

// Letter boundary lookup — morseIdx values where each letter starts
//   C=0  E=8  A=10  S=14  A=20  R=24
static const int  morseLetterStart[] = { 0, 8, 10, 14, 20, 24 };
static const char morseLetterName[]  = { 'C','E','A','S','A','R' };
static const int  morseLetterCount   = 6;

void buildMorse() {
    morseLen = 0;

    auto D  = [&]() { morse[morseLen++] = {DASH_MS,      true};  };
    auto d  = [&]() { morse[morseLen++] = {DOT_MS,       true};  };
    auto ig = [&]() { morse[morseLen++] = {INTRA_MS,     false}; };
    auto cg = [&]() { morse[morseLen++] = {INTERCHAR_MS, false}; };

    // C: −·−·
    D(); ig(); d(); ig(); D(); ig(); d(); cg();
    // E: ·
    d(); cg();
    // A: ·−
    d(); ig(); D(); cg();
    // S: ···
    d(); ig(); d(); ig(); d(); cg();
    // A: ·−
    d(); ig(); D(); cg();
    // R: ·−·  (no trailing gap — steady phase follows)
    d(); ig(); D(); ig(); d();
}

// ── Breathing serial throttle ─────────────────────────────────────────────────
unsigned long breathPrintTimer = 0;
const unsigned long BREATH_PRINT_INTERVAL = 500;

// ── LED helpers ───────────────────────────────────────────────────────────────
inline void ledOff()            { ledcWrite(PWM_CHANNEL, 0);   }
inline void ledOn()             { ledcWrite(PWM_CHANNEL, 255); }
inline void ledDuty(uint8_t v)  { ledcWrite(PWM_CHANNEL, v);   }

// ── Mode transitions ──────────────────────────────────────────────────────────
void enterOff() {
    state = STATE_OFF;
    ledOff();
    Serial.println(F("[STATE] → OFF  (LED off, waiting for next press)"));
}

void enterNextMode() {
    switch (nextMode) {
        case 0:
            state = STATE_STEADY;
            ledOn();
            nextMode = 1;
            Serial.println(F("----------------------------------------"));
            Serial.println(F("[SEQ 1] STEADY ON"));
            Serial.println(F("        Press once to turn OFF."));
            break;

        case 1:
            state      = STATE_BLINK;
            blinkState = false;
            blinkTimer = millis();
            ledOff();
            nextMode = 2;
            Serial.println(F("----------------------------------------"));
            Serial.println(F("[SEQ 2] BLINK  (300 ms interval)"));
            Serial.println(F("        Press once to turn OFF."));
            break;

        case 2:
            state       = STATE_BREATHING;
            breathStart = millis();
            breathPrintTimer = millis();
            nextMode    = 3;
            Serial.println(F("----------------------------------------"));
            Serial.println(F("[SEQ 3] BREATHING  (dim → bright, 3 s cycle)"));
            Serial.println(F("        Press once to turn OFF."));
            break;

        case 3:
            state         = STATE_MORSE;
            morseIdx      = 0;
            morseSteady   = false;
            morseSegStart = millis();
            morse[0].on ? ledOn() : ledOff();
            nextMode = 0;
            Serial.println(F("----------------------------------------"));
            Serial.println(F("[SEQ 4] MORSE  spelling: C E A S A R"));
            Serial.println(F("        Double-press within 1 s to turn OFF."));
            Serial.print  (F("        Letter: "));
            Serial.println(morseLetterName[0]);
            break;
    }
}

// ── Button reading (call once per loop) ──────────────────────────────────────
void readButton() {
    buttonPressed = false;
    doublePressed = false;

    int  raw = digitalRead(BUTTON_PIN);
    unsigned long now = millis();

    switch (btnState) {
        case BTN_IDLE:
            if (raw == HIGH) {               // button just went down
                btnState = BTN_PRESSING;
                btnTimer = now;
            }
            break;

        case BTN_PRESSING:
            if (raw == LOW) {                // glitch — too short, ignore
                btnState = BTN_IDLE;
            } else if (now - btnTimer >= DEBOUNCE_MS) {
                // Stable HIGH for DEBOUNCE_MS → confirmed press
                btnState      = BTN_HELD;
                buttonPressed = true;

                // Double-press detection
                if ((now - lastPressTime) < DOUBLE_PRESS_WINDOW) {
                    pressCount++;
                    if (pressCount >= 2) {
                        doublePressed = true;
                        pressCount    = 0;
                        Serial.println(F("[BUTTON] Double-press detected!"));
                    } else {
                        Serial.print  (F("[BUTTON] Press #"));
                        Serial.println(pressCount);
                    }
                } else {
                    pressCount = 1;
                    Serial.println(F("[BUTTON] Press #1"));
                }
                lastPressTime = now;
            }
            break;

        case BTN_HELD:
            if (raw == LOW) {                // button starting to release
                btnState = BTN_RELEASING;
                btnTimer = now;
            }
            break;

        case BTN_RELEASING:
            if (raw == HIGH) {               // bounce during release — stay held
                btnState = BTN_HELD;
            } else if (now - btnTimer >= DEBOUNCE_MS) {
                btnState = BTN_IDLE;         // fully released, ready for next press
            }
            break;
    }
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    pinMode(BUTTON_PIN, INPUT);

    ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(LED_PIN, PWM_CHANNEL);
    ledOff();

    buildMorse();

    Serial.println(F("========================================"));
    Serial.println(F("  ESP32-S3  Button → LED Controller"));
    Serial.println(F("  Button: GPIO4  |  LED: GPIO7"));
    Serial.println(F("========================================"));
    Serial.println(F("Sequences (press button to cycle):"));
    Serial.println(F("  SEQ 1 — Steady ON       (1 press to OFF)"));
    Serial.println(F("  SEQ 2 — Blink 300ms     (1 press to OFF)"));
    Serial.println(F("  SEQ 3 — Breathing 3s    (1 press to OFF)"));
    Serial.println(F("  SEQ 4 — Morse CEASAR    (double-press to OFF)"));
    Serial.println(F("========================================"));
    Serial.println(F("[STATE] → OFF  (waiting for first press)"));
}

// ── loop ──────────────────────────────────────────────────────────────────────
void loop() {
    readButton();
    unsigned long now = millis();

    switch (state) {

        // ── OFF ───────────────────────────────────────────────────────────────
        case STATE_OFF:
            if (buttonPressed) enterNextMode();
            break;

        // ── STEADY ────────────────────────────────────────────────────────────
        case STATE_STEADY:
            if (buttonPressed) enterOff();
            break;

        // ── BLINK 300 ms ──────────────────────────────────────────────────────
        case STATE_BLINK:
            if (buttonPressed) { enterOff(); break; }
            if (now - blinkTimer >= BLINK_INTERVAL) {
                blinkTimer = now;
                blinkState = !blinkState;
                blinkState ? ledOn() : ledOff();
                Serial.print(F("[BLINK] LED "));
                Serial.println(blinkState ? F("ON") : F("OFF"));
            }
            break;

        // ── BREATHING (0 → 255 linear, 3 s cycle) ────────────────────────────
        case STATE_BREATHING:
            if (buttonPressed) { enterOff(); break; }
            {
                float   t    = (float)((now - breathStart) % BREATH_CYCLE) / (float)BREATH_CYCLE;
                uint8_t duty = (uint8_t)(t * 255.0f);
                ledDuty(duty);

                if (now - breathPrintTimer >= BREATH_PRINT_INTERVAL) {
                    breathPrintTimer = now;
                    Serial.print(F("[BREATHING] Duty: "));
                    Serial.print((uint8_t)(t * 100));
                    Serial.println(F("%"));
                }
            }
            break;

        // ── MORSE "CEASAR" ────────────────────────────────────────────────────
        case STATE_MORSE:
            if (doublePressed) { enterOff(); break; }

            if (morseSteady) {
                ledOn();
                if (now - morseSteadyStart >= STEADY_MS) {
                    morseSteady   = false;
                    morseIdx      = 0;
                    morseSegStart = now;
                    morse[0].on ? ledOn() : ledOff();
                    Serial.println(F("[MORSE] Restarting — C E A S A R"));
                    Serial.print  (F("[MORSE] Letter: "));
                    Serial.println(morseLetterName[0]);
                }
            } else {
                if (now - morseSegStart >= morse[morseIdx].dur) {
                    morseIdx++;
                    morseSegStart = now;

                    if (morseIdx >= morseLen) {
                        morseSteady      = true;
                        morseSteadyStart = now;
                        ledOn();
                        Serial.println(F("[MORSE] Word complete → steady light"));
                        Serial.println(F("[MORSE] Double-press within 1 s to stop."));
                    } else {
                        // Print letter name when a new letter begins
                        for (int i = 0; i < morseLetterCount; i++) {
                            if (morseLetterStart[i] == morseIdx) {
                                Serial.print(F("[MORSE] Letter: "));
                                Serial.println(morseLetterName[i]);
                                break;
                            }
                        }
                        // Print symbol type for ON segments
                        if (morse[morseIdx].on) {
                            Serial.println(morse[morseIdx].dur == DASH_MS
                                           ? F("[MORSE]   — (dash)")
                                           : F("[MORSE]   . (dot)"));
                        }
                        morse[morseIdx].on ? ledOn() : ledOff();
                    }
                }
            }
            break;
    }
}
