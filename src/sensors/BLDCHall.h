#pragma once

#include <Arduino.h>

// Maximum number of Hall sensors (3-phase BLDC)
#define MAX_HALL_SENSORS 3

class BLDCHall {
public:
    // Single-sensor constructor (backward compatible)
    BLDCHall(uint8_t pin, uint8_t poles);

    // Multi-sensor constructor: pass an array of pins and the count
    BLDCHall(const uint8_t* pins, uint8_t numSensors, uint8_t poles);

    void begin();
    float getRPM();

    void setPoles(uint8_t poles) { _poles = poles; }

    // ISR handlers — one per channel (must be static for attachInterrupt)
    static void IRAM_ATTR handleInterrupt0();
    static void IRAM_ATTR handleInterrupt1();
    static void IRAM_ATTR handleInterrupt2();

    // Shared pulse counter: all sensors increment the same counter
    static volatile uint32_t pulseCount;
    static volatile uint32_t lastPulseTime;

private:
    uint8_t _pins[MAX_HALL_SENSORS];
    uint8_t _numSensors;
    uint8_t _poles;
    uint32_t _lastCalculateTime;
    uint32_t _lastPulseCount;
};
