#include "SensorTask.h"
#include "../config.h"
#include "../core/SystemState.h"
#include "../core/ConfigManager.h"
#include "../sensors/ZMPT101B.h"
#include "../sensors/INA226Sensor.h"
#include "../sensors/BLDCHall.h"
#include "../sensors/DS18B20Sensor.h"

// Instantiate sensors
ZMPT101B zmpt(PIN_ZMPT101B);
INA226Sensor ina(0x40, PIN_I2C_SDA, PIN_I2C_SCL); // default address 0x40
const uint8_t hallPins[BLDC_HALL_COUNT] = { PIN_BLDC_HALL_A, PIN_BLDC_HALL_B, PIN_BLDC_HALL_C };
BLDCHall hall(hallPins, BLDC_HALL_COUNT, BLDC_POLES);
TemperatureSensor temp(PIN_DS18B20);


void SensorTask::start() {
    xTaskCreatePinnedToCore(
        SensorTask::taskFunction,
        "SensorTask",
        4096,           // Stack size
        NULL,           // Parameters
        1,              // Priority
        NULL,           // Task handle
        1               // Core 1 (App Core)
    );
}

void SensorTask::taskFunction(void* pvParameters) {
    // Apply dynamic configuration
    hall.setPoles(configManager.getConfig().bldcPoles);

    // Initialization
    zmpt.begin();
    ina.begin();
    hall.begin();
    temp.begin();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(configManager.getConfig().sensorPollMs);

    for (;;) {
        // Read Sensors
        float acVoltage = zmpt.readACVoltage();
        float dcVoltage = ina.getVoltage();
        float dcCurrent = ina.getCurrent();
        float dcPower = ina.getPower();
        float rpm = hall.getRPM();
        
        temp.requestTemperature();
        float temperature = temp.getTemperature();

        // Update System State
        sysState.updateZMPT(acVoltage);
        sysState.updateINA(dcVoltage, dcCurrent, dcPower);
        sysState.updateRPM(rpm);
        sysState.updateTemp(temperature);

        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
