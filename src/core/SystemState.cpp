#include "SystemState.h"

// Instantiate the global object
SystemState sysState;

SystemState::SystemState() {
    _mutex = xSemaphoreCreateMutex();
    
    // Initialize data
    _data.dcVoltage1 = 0.0f;
    _data.dcCurrent1 = 0.0f;
    _data.dcPower1 = 0.0f;
    _data.dcVoltage2 = 0.0f;
    _data.dcCurrent2 = 0.0f;
    _data.dcPower2 = 0.0f;
    _data.acVoltage1 = 0.0f;
    _data.acVoltage2 = 0.0f;
    _data.acCurrent = 0.0f;
    _data.rpm = 0.0f;
    _data.temperature1 = 0.0f;
    _data.temperature2 = 0.0f;
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

void SystemState::updateZMPT1(float acVolts) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.acVoltage1 = acVolts;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateZMPT2(float acVolts) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.acVoltage2 = acVolts;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateZMCT(float acAmps) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.acCurrent = acAmps;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateINA1(float dcVolts, float current, float power) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.dcVoltage1 = dcVolts;
        _data.dcCurrent1 = current;
        _data.dcPower1 = power;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateINA2(float dcVolts, float current, float power) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.dcVoltage2 = dcVolts;
        _data.dcCurrent2 = current;
        _data.dcPower2 = power;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateRPM(float rpm) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.rpm = rpm;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateTemp1(float tempC) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.temperature1 = tempC;
        xSemaphoreGive(_mutex);
    }
}

void SystemState::updateTemp2(float tempC) {
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        _data.temperature2 = tempC;
        xSemaphoreGive(_mutex);
    }
}
