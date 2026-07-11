#include "DS18B20Sensor.h"

TemperatureSensor::TemperatureSensor(uint8_t pin) 
    : _pin(pin), _oneWire(pin), _sensors(&_oneWire) {
}

void TemperatureSensor::begin() {
    if (_pin >= 40) return;
    _sensors.begin();
    // Non-blocking mode (asynchronous)
    _sensors.setWaitForConversion(false);
    
    uint8_t count = _sensors.getDeviceCount();
    Serial.printf("[DS18B20] Found %d sensor(s) on pin %d\n", count, _pin);
}

void TemperatureSensor::requestTemperature() {
    if (_pin >= 40) return;
    _sensors.requestTemperatures(); // Initiate conversion for all sensors on bus
}

float TemperatureSensor::getTemperature(uint8_t index) {
    if (_pin >= 40) return 0.0f;
    float temp = _sensors.getTempCByIndex(index);
    // DallasTemperature returns DEVICE_DISCONNECTED_C (-127) if sensor not found
    if (temp == DEVICE_DISCONNECTED_C) {
        return 0.0f;
    }
    return temp;
}

uint8_t TemperatureSensor::getDeviceCount() {
    if (_pin >= 40) return 0;
    return _sensors.getDeviceCount();
}
