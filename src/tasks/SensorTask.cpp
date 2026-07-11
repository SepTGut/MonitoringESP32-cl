#include "SensorTask.h"
#include "../config.h"
#include "../core/SystemState.h"
#include "../core/ConfigManager.h"
#include "../sensors/ZMPT101B.h"
#include "../sensors/ZMCT103C.h"
#include "../sensors/INA226Sensor.h"
#include "../sensors/BLDCHall.h"
#include "../sensors/DS18B20Sensor.h"
#include <Wire.h>

// Instantiate sensors
ZMPT101B zmpt1(PIN_ZMPT101B_1);
ZMPT101B zmpt2(PIN_ZMPT101B_2);
ZMCT103C zmct(PIN_ZMCT103C);
INA226Sensor ina1(INA226_ADDR_1, PIN_I2C_SDA, PIN_I2C_SCL);
INA226Sensor ina2(INA226_ADDR_2, PIN_I2C_SDA, PIN_I2C_SCL);
BLDCHall hall;
TemperatureSensor tempBus(PIN_DS18B20);

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
    AppConfig cfg = configManager.getConfig();

    // Initialization
    zmpt1.begin();
    zmpt2.begin();
    zmct.begin();
    
    // Initialize I2C bus once before INA226 modules (avoids double Wire.begin())
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    ina1.begin();
    ina2.begin();
    
    // Initialize Hall with configured mode and poles
    hall.begin(cfg.rpmMode, cfg.bldcPoles);
    
    tempBus.begin();
    // Issue the first temperature conversion request so data is ready on the first read
    tempBus.requestTemperature();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(cfg.sensorPollMs);

    for (;;) {
        // Read DS18B20 temperatures (conversion was requested in the PREVIOUS iteration)
        float temperature1 = tempBus.getTemperature(0);
        float temperature2 = tempBus.getTemperature(1);

        // Read AC Sensors
        float acVoltage1 = zmpt1.readACVoltage();
        float acVoltage2 = zmpt2.readACVoltage();
        float acCurrent = zmct.readACCurrent();
        
        float dcVoltage1 = ina1.getVoltage();
        float dcCurrent1 = ina1.getCurrent();
        float dcPower1 = ina1.getPower();
        
        float dcVoltage2 = ina2.getVoltage();
        float dcCurrent2 = ina2.getCurrent();
        float dcPower2 = ina2.getPower();
        
        float rpm = hall.getRPM();

        // Request next DS18B20 conversion NOW — it will complete during vTaskDelayUntil
        tempBus.requestTemperature();

        // Update System State
        sysState.updateZMPT1(acVoltage1);
        sysState.updateZMPT2(acVoltage2);
        sysState.updateZMCT(acCurrent);
        sysState.updateINA1(dcVoltage1, dcCurrent1, dcPower1);
        sysState.updateINA2(dcVoltage2, dcCurrent2, dcPower2);
        sysState.updateRPM(rpm);
        sysState.updateTemp1(temperature1);
        sysState.updateTemp2(temperature2);

        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
