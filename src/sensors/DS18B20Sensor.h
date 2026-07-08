#pragma once

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

class TemperatureSensor {
public:
    TemperatureSensor(uint8_t pin);
    void begin();
    float getTemperature();
    void requestTemperature();

private:
    uint8_t _pin;
    OneWire _oneWire;
    DallasTemperature _sensors;
};
