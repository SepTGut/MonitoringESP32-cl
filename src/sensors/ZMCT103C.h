#pragma once

#include <Arduino.h>

class ZMCT103C {
public:
    ZMCT103C(uint8_t pin);
    void begin();
    float readACCurrent();  // Returns RMS amps

private:
    uint8_t _pin;
    float _calibrationFactor;
    uint16_t getPeakToPeak();
};
