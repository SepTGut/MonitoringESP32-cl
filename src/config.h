#pragma once

#include <Arduino.h>

// --- PIN DEFINITIONS ---
// Use ADC1 for ZMPT101B (ADC2 is used by Wi-Fi)
#define PIN_ZMPT101B 32 // ADC1_CH4

// I2C Pins (INA226)
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// BLDC Hall Sensors — 3-phase (Interrupt-capable pins)
#define PIN_BLDC_HALL_A 34 // Input only, suitable for external pull-up
#define PIN_BLDC_HALL_B 35 // Input only
#define PIN_BLDC_HALL_C 39 // Input only (VN)
#define BLDC_HALL_COUNT 3  // Number of Hall sensors wired

// DS18B20 1-Wire Pin
#define PIN_DS18B20 4

// --- CONFIGURATIONS ---
#define WIFI_SSID "WindTurbine_AP"
#define WIFI_PASS "12345678"

#define SENSOR_POLL_MS 100    // 10Hz sampling
#define WEBSOCKET_PUSH_MS 500 // 2Hz dashboard updates

#define BLDC_POLES 4 // Number of magnetic poles on the BLDC motor

// --- FEATURE FLAGS ---
// #define ENABLE_SD_LOGGING
// #define ENABLE_MQTT
