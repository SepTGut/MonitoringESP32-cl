#include "BLDCHall.h"

volatile uint32_t BLDCHall::pulseCount = 0;
volatile uint32_t BLDCHall::lastPulseTime = 0;

void IRAM_ATTR bldc_isr() {
    BLDCHall::pulseCount++;
    BLDCHall::lastPulseTime = millis(); // Simple debounce could be added here
}

BLDCHall::BLDCHall(uint8_t pin, uint8_t poles) : _pin(pin), _poles(poles) {
    _lastCalculateTime = 0;
    _lastPulseCount = 0;
}

void BLDCHall::begin() {
    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(_pin), bldc_isr, RISING);
}

float BLDCHall::getRPM() {
    uint32_t currentMillis = millis();
    uint32_t currentPulses = pulseCount;
    
    // Protection against division by zero or too short interval
    uint32_t timeDelta = currentMillis - _lastCalculateTime;
    if (timeDelta < 10) return 0.0f; // Too fast

    uint32_t pulses = currentPulses - _lastPulseCount;
    
    // (Pulses / timeDelta in ms) * 1000 = pulses/sec
    // pulses/sec * 60 = pulses/min
    // (pulses/min) / poles = RPM
    float rpm = ((float)pulses / timeDelta) * 1000.0f * 60.0f / (float)_poles;

    _lastCalculateTime = currentMillis;
    _lastPulseCount = currentPulses;

    // Zero out RPM if no pulses for a while (e.g. 1 second)
    if (currentMillis - lastPulseTime > 1000) {
        rpm = 0.0f;
    }

    return rpm;
}
