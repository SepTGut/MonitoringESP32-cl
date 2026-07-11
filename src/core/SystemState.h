#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct SensorData {
    // INA226 × 2 (DC channels)
    float dcVoltage1;      // INA226 #1
    float dcCurrent1;
    float dcPower1;
    float dcVoltage2;      // INA226 #2
    float dcCurrent2;
    float dcPower2;

    // ZMPT101B × 2 (AC Voltage channels)
    float acVoltage1;      // ZMPT101B #1
    float acVoltage2;      // ZMPT101B #2

    // ZMCT103C × 1 (AC Current)
    float acCurrent;       // ZMCT103C

    // RPM
    float rpm;

    // DS18B20 × 2
    float temperature1;    // DS18B20 #1
    float temperature2;    // DS18B20 #2
};

class SystemState {
public:
    SystemState();
    ~SystemState();

    // Get a copy of the current state
    SensorData getData();
    
    // Update individual values (thread-safe)
    void updateZMPT1(float acVolts);
    void updateZMPT2(float acVolts);
    void updateZMCT(float acAmps);
    void updateINA1(float dcVolts, float current, float power);
    void updateINA2(float dcVolts, float current, float power);
    void updateRPM(float rpm);
    void updateTemp1(float tempC);
    void updateTemp2(float tempC);

private:
    SensorData _data;
    SemaphoreHandle_t _mutex;
};

// Global instance defined in cpp
extern SystemState sysState;
