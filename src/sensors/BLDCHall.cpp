#include "BLDCHall.h"

volatile uint32_t BLDCHall::pulseCount = 0;
volatile uint32_t BLDCHall::lastPulseTime = 0;

// --- ISR handlers (one per channel, all increment the shared counter) ---
void IRAM_ATTR BLDCHall::handleInterrupt0() {
    pulseCount++;
    lastPulseTime = millis();
}

void IRAM_ATTR BLDCHall::handleInterrupt1() {
    pulseCount++;
    lastPulseTime = millis();
}

void IRAM_ATTR BLDCHall::handleInterrupt2() {
    pulseCount++;
    lastPulseTime = millis();
}

// Function pointer table for attachInterrupt (indexed by sensor index)
static void (*isr_table[MAX_HALL_SENSORS])() = {
    BLDCHall::handleInterrupt0,
    BLDCHall::handleInterrupt1,
    BLDCHall::handleInterrupt2
};

// Default constructor
BLDCHall::BLDCHall()
    : _numSensors(3), _poles(4), _mode(0), _lastCalculateTime(0), _lastPulseCount(0) {
    _pins[0] = 34; // PIN_RPM_INPUT
    _pins[1] = 35; // PIN_BLDC_HALL_B
    _pins[2] = 39; // PIN_BLDC_HALL_C
}

// Single-sensor constructor (backward compatible)
BLDCHall::BLDCHall(uint8_t pin, uint8_t poles)
    : _numSensors(1), _poles(poles), _mode(1), _lastCalculateTime(0), _lastPulseCount(0) {
    _pins[0] = pin;
    _pins[1] = 0;
    _pins[2] = 0;
}

// Multi-sensor constructor
BLDCHall::BLDCHall(const uint8_t* pins, uint8_t numSensors, uint8_t poles)
    : _numSensors(numSensors > MAX_HALL_SENSORS ? MAX_HALL_SENSORS : numSensors),
      _poles(poles), _mode(0), _lastCalculateTime(0), _lastPulseCount(0) {
    for (uint8_t i = 0; i < _numSensors; i++) {
        _pins[i] = pins[i];
    }
}

void BLDCHall::begin() {
    begin(_mode, _poles);
}

void BLDCHall::begin(uint8_t mode, uint8_t poles) {
    // First detach any existing interrupts on these pins to avoid duplicates/crashes on re-init
    for (uint8_t pin : {34, 35, 39}) {
        if (pin < 40) {
            int intr = digitalPinToInterrupt(pin);
            if (intr != -1) {
                detachInterrupt(intr);
            }
        }
    }

    _poles = poles;
    _mode = mode;

    if (_mode == 0) { // RPM_MODE_HALL_3
        _numSensors = 3;
        _pins[0] = 34; // PIN_RPM_INPUT
        _pins[1] = 35; // PIN_BLDC_HALL_B
        _pins[2] = 39; // PIN_BLDC_HALL_C
    } else if (_mode == 3) { // RPM_MODE_HALL_2
        _numSensors = 2;
        _pins[0] = 34; // PIN_RPM_INPUT  (Hall A)
        _pins[1] = 35; // PIN_BLDC_HALL_B (Hall B)
    } else { // RPM_MODE_HALL_1 or RPM_MODE_IR
        _numSensors = 1;
        _pins[0] = 34; // PIN_RPM_INPUT
    }

    pulseCount = 0;
    _lastPulseCount = 0;
    _lastCalculateTime = millis();
    lastPulseTime = 0;

    for (uint8_t i = 0; i < _numSensors; i++) {
        if (_pins[i] < 40) {
            pinMode(_pins[i], INPUT_PULLUP);
            int intr = digitalPinToInterrupt(_pins[i]);
            if (intr != -1) {
                attachInterrupt(intr, isr_table[i], RISING);
            }
        }
    }
    Serial.printf("[BLDCHall] Initialized in mode %d with %d sensor(s) on pins:", _mode, _numSensors);
    for (uint8_t i = 0; i < _numSensors; i++) {
        if (_pins[i] < 40) {
            Serial.printf(" GPIO%d", _pins[i]);
        } else {
            Serial.print(" DISABLED");
        }
    }
    Serial.println();
}

float BLDCHall::getRPM() {
    uint32_t currentMillis = millis();
    uint32_t currentPulses = pulseCount;

    // Protection against division by zero or too short interval
    uint32_t timeDelta = currentMillis - _lastCalculateTime;
    if (timeDelta < 10) return 0.0f; // Too fast

    uint32_t pulses = currentPulses - _lastPulseCount;

    float pulsesPerRev = 1.0f;
    if (_mode == 0) { // RPM_MODE_HALL_3
        // With 3 sensors, each revolution generates (poles * 3) pulses
        pulsesPerRev = (float)_poles * 3.0f;
    } else if (_mode == 3) { // RPM_MODE_HALL_2
        // With 2 sensors, each revolution generates (poles * 2) pulses
        pulsesPerRev = (float)_poles * 2.0f;
    } else if (_mode == 1) { // RPM_MODE_HALL_1
        // With 1 Hall sensor triggering on rotor magnets
        pulsesPerRev = (float)_poles;
    } else { // RPM_MODE_IR (IR tachometer)
        // Typically 1 pulse per revolution
        pulsesPerRev = 1.0f;
    }

    float rpm = ((float)pulses / timeDelta) * 1000.0f * 60.0f / pulsesPerRev;

    _lastCalculateTime = currentMillis;
    _lastPulseCount = currentPulses;

    // Zero out RPM if no pulses for a while (e.g. 1 second)
    if (currentMillis - lastPulseTime > 1000) {
        rpm = 0.0f;
    }

    return rpm;
}
