#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

struct AppConfig {
    char wifiSSID[33];
    char wifiPass[64];
    uint32_t sensorPollMs;
    uint32_t wsPushMs;
    uint32_t bldcPoles;
    
    // WiFi Station (STA) Client Mode configs
    bool staEnabled;
    char staSSID[33];
    char staPass[64];

    // MQTT Broker configs
    bool mqttEnabled;
    char mqttServer[64];
    uint16_t mqttPort;
    char mqttUser[33];
    char mqttPass[64];
    char mqttTopic[64];
    uint32_t mqttInterval;

    // Display / Limit configs
    float maxV;
    float maxA;
    uint32_t maxRPM;
    uint32_t maxTemp;
};

class ConfigManager {
public:
    ConfigManager();
    bool begin();
    bool load();
    bool save();
    
    const AppConfig& getConfig() const { return _config; }
    void updateConfig(const AppConfig& newConfig);
    void updateFromJson(const JsonVariant& json);
    void serialize(JsonDocument& doc) const;

private:
    AppConfig _config;
    const char* _filename = "/config.json";
};

extern ConfigManager configManager;
