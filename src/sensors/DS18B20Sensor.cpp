#include "DS18B20Sensor.h"

TemperatureSensor::TemperatureSensor(uint8_t pin) 
    : _pin(pin), _oneWire(pin), _sensors(&_oneWire) {
}

void TemperatureSensor::begin() {
    _sensors.begin();
    // Non-blocking mode (asynchronous)
    _sensors.setWaitForConversion(false);
}

void TemperatureSensor::requestTemperature() {
    _sensors.requestTemperatures(); // Initiate conversion
}

float TemperatureSensor::getTemperature() {
    // Note: getTempCByIndex returns 85.0 if conversion isn't complete, 
    // but with waitForConversion(false) you need to manage timing yourself (e.g., wait 750ms between request and read)
    return _sensors.getTempCByIndex(0);
}
