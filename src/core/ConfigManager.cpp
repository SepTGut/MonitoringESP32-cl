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
    doc["maxV"] = _config.maxV;
    doc["maxA"] = _config.maxA;
    doc["maxRPM"] = _config.maxRPM;
    doc["maxTemp"] = _config.maxTemp;
}
