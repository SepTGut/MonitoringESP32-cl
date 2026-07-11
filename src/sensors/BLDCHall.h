#pragma once

#include <Arduino.h>

// Maximum number of Hall sensors (3-phase BLDC)
#define MAX_HALL_SENSORS 3

class BLDCHall {
public:
    // Constructor
    BLDCHall();

    // Backward-compatible constructors (delegating to defaults)
    BLDCHall(uint8_t pin, uint8_t poles);
    BLDCHall(const uint8_t* pins, uint8_t numSensors, uint8_t poles);

    void begin();
    void begin(uint8_t mode, uint8_t poles);
    float getRPM();

    void setPoles(uint8_t poles) { _poles = poles; }
    void setMode(uint8_t mode) { _mode = mode; }

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
    uint8_t _mode;
    uint32_t _lastCalculateTime;
    uint32_t _lastPulseCount;
};
