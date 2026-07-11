#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class TemperatureSensor {
public:
    TemperatureSensor(uint8_t pin);
    void begin();
    void requestTemperature();

    // Read temperature by sensor index on the OneWire bus
    float getTemperature(uint8_t index = 0);

    // Get the number of sensors detected on the bus
    uint8_t getDeviceCount();

private:
    uint8_t _pin;
    OneWire _oneWire;
    DallasTemperature _sensors;
};
