#include "INA226Sensor.h"

INA226Sensor::INA226Sensor(uint8_t address, uint8_t sda, uint8_t scl) 
    : _ina(address), _address(address), _sda(sda), _scl(scl) {
}

bool INA226Sensor::begin() {
    Wire.begin(_sda, _scl);
    if (!_ina.begin()) {
        Serial.println("could not connect to INA226");
        return false;
    }
    _ina.setMaxCurrentShunt(10.0, 0.01); // e.g., max 10A, 0.01 ohm shunt
    return true;
}

float INA226Sensor::getVoltage() {
    return _ina.getBusVoltage();
}

float INA226Sensor::getCurrent() {
    return _ina.getCurrent_mA() / 1000.0f; // Return A
}

float INA226Sensor::getPower() {
    return _ina.getPower_mW() / 1000.0f;   // Return W
}
