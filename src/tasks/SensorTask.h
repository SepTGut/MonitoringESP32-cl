#pragma once

#include <Arduino.h>

class SensorTask {
public:
    static void start();
private:
    static void taskFunction(void* pvParameters);
};
