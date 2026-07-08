#include <Arduino.h>
#include "core/ConfigManager.h"
#include "tasks/SensorTask.h"
#include "tasks/NetTask.h"

void setup() {
    Serial.begin(115200);
    Serial.println("\n--- Portable Wind Turbine Monitoring System ---");

    // Initialize configuration manager (loads settings from LittleFS)
    configManager.begin();

    // Start FreeRTOS Tasks
    SensorTask::start();
    NetTask::start();

    // setup() and loop() run on Core 1 by default, but we've delegated 
    // functionality to dedicated tasks. We can simply delete the setup/loop task
    // or let loop() be idle.
    vTaskDelete(NULL); 
}

void loop() {
    // Should never reach here due to vTaskDelete(NULL)
}
