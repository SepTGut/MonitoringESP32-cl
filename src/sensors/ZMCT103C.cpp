#include "ZMCT103C.h"

ZMCT103C::ZMCT103C(uint8_t pin) : _pin(pin), _calibrationFactor(1.0f) {
}

void ZMCT103C::begin() {
    if (_pin >= 40) return;
    pinMode(_pin, INPUT);
    // Calibration factor depends on burden resistor and turns ratio.
    // ZMCT103C typical: 1000:1 turns ratio with a 200Ω burden resistor
    // → 1A primary = 1mA secondary = 0.2V across burden
    // Adjust this value based on your actual circuit.
    _calibrationFactor = 1.0;
}

float ZMCT103C::readACCurrent() {
    if (_pin >= 40) return 0.0f;
    uint16_t peakToPeak = getPeakToPeak();
    // Convert ADC peak-to-peak to voltage, then to RMS
    float voltage = (peakToPeak * 3.3f / 4095.0f) / 2.0f * 0.707f; // Vrms across burden
    // Convert burden voltage to current using calibration factor
    // Default assumes 200Ω burden: I = V / R, but calibration factor absorbs this
    return voltage * _calibrationFactor * 5.0f; // Adjust multiplier for your burden resistor
}

uint16_t ZMCT103C::getPeakToPeak() {
    uint16_t signalMax = 0;
    uint16_t signalMin = 4095;
    uint32_t startMillis = millis();

    // Sample for at least one full 50Hz/60Hz cycle (~25ms)
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
