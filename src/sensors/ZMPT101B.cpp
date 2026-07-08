#include "ZMPT101B.h"

ZMPT101B::ZMPT101B(uint8_t pin) : _pin(pin), _calibrationFactor(1.0f) {
}

void ZMPT101B::begin() {
    pinMode(_pin, INPUT);
    // You might want to calculate the calibration factor based on a known voltage
    _calibrationFactor = 1.5; // Example value, needs calibration
}

float ZMPT101B::readACVoltage() {
    uint16_t peakToPeak = getPeakToPeak();
    float voltage = (peakToPeak * 3.3 / 4095.0) / 2.0 * 0.707; // Vrms
    return voltage * _calibrationFactor * 100.0; // Assuming 100:1 transformer or similar mapping
}

uint16_t ZMPT101B::getPeakToPeak() {
    uint16_t signalMax = 0;
    uint16_t signalMin = 4095;
    uint32_t startMillis = millis();
    
    // Sample for at least one full 50Hz/60Hz cycle (e.g., 20ms)
    while (millis() - startMillis < 25) {
        uint16_t sample = analogRead(_pin);
        if (sample > signalMax) {
            signalMax = sample;
        }
        if (sample < signalMin) {
            signalMin = sample;
        }
    }
    return signalMax - signalMin;
}
