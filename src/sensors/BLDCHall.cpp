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

// Single-sensor constructor (backward compatible)
BLDCHall::BLDCHall(uint8_t pin, uint8_t poles)
    : _numSensors(1), _poles(poles), _lastCalculateTime(0), _lastPulseCount(0) {
    _pins[0] = pin;
    _pins[1] = 0;
    _pins[2] = 0;
}

// Multi-sensor constructor
BLDCHall::BLDCHall(const uint8_t* pins, uint8_t numSensors, uint8_t poles)
    : _numSensors(numSensors > MAX_HALL_SENSORS ? MAX_HALL_SENSORS : numSensors),
      _poles(poles), _lastCalculateTime(0), _lastPulseCount(0) {
    for (uint8_t i = 0; i < _numSensors; i++) {
        _pins[i] = pins[i];
    }
}

void BLDCHall::begin() {
    for (uint8_t i = 0; i < _numSensors; i++) {
        pinMode(_pins[i], INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(_pins[i]), isr_table[i], RISING);
    }
    Serial.printf("[BLDCHall] %d Hall sensor(s) initialized", _numSensors);
    for (uint8_t i = 0; i < _numSensors; i++) {
        Serial.printf(" GPIO%d", _pins[i]);
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

    // With N sensors on a BLDC, each revolution generates (poles × N) pulses.
    // RPM = (pulses / timeDelta_ms) × 1000 × 60 / (poles × numSensors)
    float pulsesPerRev = (float)_poles * (float)_numSensors;
    float rpm = ((float)pulses / timeDelta) * 1000.0f * 60.0f / pulsesPerRev;

    _lastCalculateTime = currentMillis;
    _lastPulseCount = currentPulses;

    // Zero out RPM if no pulses for a while (e.g. 1 second)
    if (currentMillis - lastPulseTime > 1000) {
        rpm = 0.0f;
    }

    return rpm;
}
