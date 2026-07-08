#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// Struct to hold all the latest sensor data
struct SensorData {
    float acVoltage;       // From ZMPT101B
    float dcVoltage;       // From INA226
    float dcCurrent;       // From INA226
    float dcPower;         // Computed or from INA226
    float rpm;             // From BLDC Hall sensor
    float temperatureC;    // From DS18B20
};

class SystemState {
public:
    SystemState();
    ~SystemState();

    // Get a copy of the current state
    SensorData getData();
    
    // Update individual values (thread-safe)
    void updateZMPT(float acVolts);
    void updateINA(float dcVolts, float current, float power);
    void updateRPM(float rpm);
    void updateTemp(float tempC);

private:
    SensorData _data;
    SemaphoreHandle_t _mutex;
};

// Global instance defined in cpp
extern SystemState sysState;
