#include "ConfigManager.h"
#include <LittleFS.h>
#include "../config.h"

ConfigManager configManager;

ConfigManager::ConfigManager() {
    // Set defaults from config.h
    strncpy(_config.wifiSSID, WIFI_SSID, sizeof(_config.wifiSSID) - 1);
    _config.wifiSSID[sizeof(_config.wifiSSID) - 1] = '\0';
    
    strncpy(_config.wifiPass, WIFI_PASS, sizeof(_config.wifiPass) - 1);
    _config.wifiPass[sizeof(_config.wifiPass) - 1] = '\0';
    
    _config.sensorPollMs = SENSOR_POLL_MS;
    _config.wsPushMs = WEBSOCKET_PUSH_MS;
    _config.bldcPoles = BLDC_POLES;
    
    // Station and MQTT defaults
    _config.staEnabled = false;
    _config.staSSID[0] = '\0';
    _config.staPass[0] = '\0';

    _config.mqttEnabled = false;
    _config.mqttServer[0] = '\0';
    _config.mqttPort = 1883;
    _config.mqttUser[0] = '\0';
    _config.mqttPass[0] = '\0';
    strncpy(_config.mqttTopic, "sapa/turbine/metrics", sizeof(_config.mqttTopic) - 1);
    _config.mqttTopic[sizeof(_config.mqttTopic) - 1] = '\0';
    _config.mqttInterval = 5000;

    _config.maxV = 60.0f;
    _config.maxA = 20.0f;
    _config.maxRPM = 3000;
    _config.maxTemp = 100;
}

bool ConfigManager::begin() {
    // LittleFS initialization is handled by DashboardServer, but we can verify or mount here as well
    if (!LittleFS.begin(true)) {
        Serial.println("[Config] Error mounting LittleFS");
        return false;
    }
    return load();
}

bool ConfigManager::load() {
    if (!LittleFS.exists(_filename)) {
        Serial.println("[Config] Config file not found. Saving defaults.");
        save();
        return true;
    }

    File configFile = LittleFS.open(_filename, "r");
    if (!configFile) {
        Serial.println("[Config] Failed to open config file for reading");
        return false;
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    configFile.close();

    if (error) {
        Serial.println("[Config] Failed to parse config file, using defaults");
        return false;
    }

    updateFromJson(doc.as<JsonVariant>());
    Serial.println("[Config] Loaded configuration successfully");
    return true;
}

bool ConfigManager::save() {
    File configFile = LittleFS.open(_filename, "w");
    if (!configFile) {
        Serial.println("[Config] Failed to open config file for writing");
        return false;
    }

    StaticJsonDocument<512> doc;
    serialize(doc);

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("[Config] Failed to write config to file");
        configFile.close();
        return false;
    }

    configFile.close();
    Serial.println("[Config] Saved configuration successfully");
    return true;
}

void ConfigManager::updateConfig(const AppConfig& newConfig) {
    _config = newConfig;
}

void ConfigManager::updateFromJson(const JsonVariant& json) {
    if (json["ssid"].is<const char*>()) {
        strncpy(_config.wifiSSID, json["ssid"], sizeof(_config.wifiSSID) - 1);
        _config.wifiSSID[sizeof(_config.wifiSSID) - 1] = '\0';
    }
    if (json["pass"].is<const char*>()) {
        strncpy(_config.wifiPass, json["pass"], sizeof(_config.wifiPass) - 1);
        _config.wifiPass[sizeof(_config.wifiPass) - 1] = '\0';
    }
    if (json["pollMs"].is<uint32_t>()) {
        _config.sensorPollMs = json["pollMs"];
    }
    if (json["wsPushMs"].is<uint32_t>()) {
        _config.wsPushMs = json["wsPushMs"];
    }
    if (json["poles"].is<uint32_t>()) {
        _config.bldcPoles = json["poles"];
    }

    // WiFi STA config deserialization
    if (json["staEnabled"].is<bool>()) {
        _config.staEnabled = json["staEnabled"];
    }
    if (json["staSSID"].is<const char*>()) {
        strncpy(_config.staSSID, json["staSSID"], sizeof(_config.staSSID) - 1);
        _config.staSSID[sizeof(_config.staSSID) - 1] = '\0';
    }
    if (json["staPass"].is<const char*>()) {
        strncpy(_config.staPass, json["staPass"], sizeof(_config.staPass) - 1);
        _config.staPass[sizeof(_config.staPass) - 1] = '\0';
    }

    // MQTT config deserialization
    if (json["mqttEnabled"].is<bool>()) {
        _config.mqttEnabled = json["mqttEnabled"];
    }
    if (json["mqttServer"].is<const char*>()) {
        strncpy(_config.mqttServer, json["mqttServer"], sizeof(_config.mqttServer) - 1);
        _config.mqttServer[sizeof(_config.mqttServer) - 1] = '\0';
    }
    if (json["mqttPort"].is<uint16_t>()) {
        _config.mqttPort = json["mqttPort"];
    }
    if (json["mqttUser"].is<const char*>()) {
        strncpy(_config.mqttUser, json["mqttUser"], sizeof(_config.mqttUser) - 1);
        _config.mqttUser[sizeof(_config.mqttUser) - 1] = '\0';
    }
    if (json["mqttPass"].is<const char*>()) {
        strncpy(_config.mqttPass, json["mqttPass"], sizeof(_config.mqttPass) - 1);
        _config.mqttPass[sizeof(_config.mqttPass) - 1] = '\0';
    }
    if (json["mqttTopic"].is<const char*>()) {
        strncpy(_config.mqttTopic, json["mqttTopic"], sizeof(_config.mqttTopic) - 1);
        _config.mqttTopic[sizeof(_config.mqttTopic) - 1] = '\0';
    }
    if (json["mqttInterval"].is<uint32_t>()) {
        _config.mqttInterval = json["mqttInterval"];
    }

    if (json["maxV"].is<float>()) {
        _config.maxV = json["maxV"];
    }
    if (json["maxA"].is<float>()) {
        _config.maxA = json["maxA"];
    }
    if (json["maxRPM"].is<uint32_t>()) {
        _config.maxRPM = json["maxRPM"];
    }
    if (json["maxTemp"].is<uint32_t>()) {
        _config.maxTemp = json["maxTemp"];
    }
}

void ConfigManager::serialize(JsonDocument& doc) const {
    doc["ssid"] = _config.wifiSSID;
    doc["pass"] = _config.wifiPass;
    doc["pollMs"] = _config.sensorPollMs;
    doc["wsPushMs"] = _config.wsPushMs;
    doc["poles"] = _config.bldcPoles;

    // WiFi STA config serialization
    doc["staEnabled"] = _config.staEnabled;
    doc["staSSID"] = _config.staSSID;
    doc["staPass"] = _config.staPass;

    // MQTT config serialization
    doc["mqttEnabled"] = _config.mqttEnabled;
    doc["mqttServer"] = _config.mqttServer;
    doc["mqttPort"] = _config.mqttPort;
    doc["mqttUser"] = _config.mqttUser;
    doc["mqttPass"] = _config.mqttPass;
    doc["mqttTopic"] = _config.mqttTopic;
    doc["mqttInterval"] = _config.mqttInterval;

    doc["maxV"] = _config.maxV;
    doc["maxA"] = _config.maxA;
    doc["maxRPM"] = _config.maxRPM;
    doc["maxTemp"] = _config.maxTemp;
}
