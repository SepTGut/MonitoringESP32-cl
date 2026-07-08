#include "SystemState.h"

// Instantiate the global object
SystemState sysState;

SystemState::SystemState() {
    _mutex = xSemaphoreCreateMutex();
    
    // Initialize data
    _data.acVoltage = 0.0f;
    _data.dcVoltage = 0.0f;
    _data.dcCurrent = 0.0f;
    _data.dcPower = 0.0f;
    _data.rpm = 0.0f;
    _data.temperatureC = 0.0f;
}

SystemState::~SystemState() {
    if (_mutex != NULL) {
        vSemaphoreDelete(_mutex);
    }
}

SensorData SystemState::getData() {
    SensorData copy;
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        copy = _data;
        xSemaphoreGive(_mutex);
    }
    return copy;
}

void SystemState::updateZMPT(float acVolts) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.acVoltage = acVolts;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateINA(float dcVolts, float current, float power) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.dcVoltage = dcVolts;
        _data.dcCurrent = current;
        _data.dcPower = power;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateRPM(float rpm) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.rpm = rpm;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateTemp(float tempC) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.temperatureC = tempC;
        xSemaphoreGive(_mutex);
    }
}
