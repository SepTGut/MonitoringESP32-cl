#pragma once

#include <Arduino.h>

// --- PIN DEFINITIONS ---

// ZMPT101B AC Voltage Sensors (ADC1 only — ADC2 is used by Wi-Fi)
#define PIN_ZMPT101B_1  32   // ADC1_CH4 — AC Voltage channel 1
#define PIN_ZMPT101B_2  33   // ADC1_CH5 — AC Voltage channel 2

// ZMCT103C AC Current Sensor
#define PIN_ZMCT103C    36   // ADC1_CH0 (VP) — AC Current

// I2C Pins (shared bus for INA226 × 2)
#define PIN_I2C_SDA     21
#define PIN_I2C_SCL     22
#define INA226_ADDR_1   0x40  // INA226 #1 (A0=GND, A1=GND)
#define INA226_ADDR_2   0x41  // INA226 #2 (A0=VS,  A1=GND)

// RPM Input Pins (interrupt-capable, input-only)
#define PIN_RPM_INPUT   34   // Primary: Hall A / External Hall / IR module
#define PIN_BLDC_HALL_B 35   // 3-phase mode only: Hall B
#define PIN_BLDC_HALL_C 39   // 3-phase mode only: Hall C (VN)

// RPM Mode Constants
#define RPM_MODE_HALL_3    0  // 3-phase internal hall (3 pins)
#define RPM_MODE_HALL_1    1  // Single external hall sensor
#define RPM_MODE_IR        2  // Infrared RPM module
#define RPM_MODE_HALL_2    3  // 2-phase internal hall (2 pins)

// DS18B20 1-Wire Pin (both sensors on same bus)
#define PIN_DS18B20     4

// --- CONFIGURATIONS ---
#define WIFI_SSID       "Monitor_SaPa_AP"
#define WIFI_PASS       "12345678"

#define SENSOR_POLL_MS      100    // 10Hz sampling
#define WEBSOCKET_PUSH_MS   500    // 2Hz dashboard updates

#define BLDC_POLES      12 // Number of magnetic poles (rotor magnets) — LG WD-M1070D6 DD motor

// --- FEATURE FLAGS ---
// #define ENABLE_SD_LOGGING
// #define ENABLE_MQTT
