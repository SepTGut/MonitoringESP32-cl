#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <INA226.h> // Uses robtillaart/INA226

class INA226Sensor {
public:
    INA226Sensor(uint8_t address, uint8_t sda, uint8_t scl);
    bool begin();
    
    float getVoltage();
    float getCurrent();
    float getPower();

private:
    INA226 _ina;
    uint8_t _address;
    uint8_t _sda;
    uint8_t _scl;
};
