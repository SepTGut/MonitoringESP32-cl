#pragma once

#include <Arduino.h>

class BLDCHall {
public:
    BLDCHall(uint8_t pin, uint8_t poles);
    void begin();
    float getRPM();

    // Must be static/global or IRAM_ATTR friendly for attachInterrupt
    void setPoles(uint8_t poles) { _poles = poles; }
    static void handleInterrupt();
    static volatile uint32_t pulseCount;
    static volatile uint32_t lastPulseTime;
    
private:
    uint8_t _pin;
    uint8_t _poles;
    uint32_t _lastCalculateTime;
    uint32_t _lastPulseCount;
};
