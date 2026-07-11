#include "INA226Sensor.h"

INA226Sensor::INA226Sensor(uint8_t address, uint8_t sda, uint8_t scl) 
    : _ina(address), _address(address), _sda(sda), _scl(scl), _enabled(false) {
}

bool INA226Sensor::begin() {
    if (_address == 0 || _address == 255 || _sda >= 40 || _scl >= 40) {
        _enabled = false;
        return false;
    }
    // Note: Wire.begin() is called once in SensorTask before INA226 init
    if (!_ina.begin()) {
        Serial.println("could not connect to INA226");
        _enabled = false;
        return false;
    }
    _ina.setMaxCurrentShunt(10.0, 0.01); // e.g., max 10A, 0.01 ohm shunt
    _enabled = true;
    return true;
}

float INA226Sensor::getVoltage() {
    if (!_enabled) return 0.0f;
    return _ina.getBusVoltage();
}

float INA226Sensor::getCurrent() {
    if (!_enabled) return 0.0f;
    return _ina.getCurrent_mA() / 1000.0f; // Return A
}

float INA226Sensor::getPower() {
    if (!_enabled) return 0.0f;
    return _ina.getPower_mW() / 1000.0f;   // Return W
}
