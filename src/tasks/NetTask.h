#pragma once

#include <Arduino.h>

class NetTask {
public:
    static void start();
private:
    static void taskFunction(void* pvParameters);
};
