#pragma once

#include <Arduino.h>

class ZMPT101B {
public:
    ZMPT101B(uint8_t pin);
    void begin();
    float readACVoltage();

private:
    uint8_t _pin;
    float _calibrationFactor;
    // Helper to get raw analog AC peak-to-peak
    uint16_t getPeakToPeak();
};
