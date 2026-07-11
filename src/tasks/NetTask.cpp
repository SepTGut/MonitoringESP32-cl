#include "NetTask.h"
#include "../config.h"
#include "../net/WebServer.h"
#include "../core/ConfigManager.h"
#include "../core/SystemState.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

void NetTask::start() {
    xTaskCreatePinnedToCore(
        NetTask::taskFunction,
        "NetTask",
        8192,           // Larger stack for network, web server and MQTT
        NULL,           // Parameters
        1,              // Priority
        NULL,           // Task handle
        0               // Core 0 (Protocol Core)
    );
}

void NetTask::taskFunction(void* pvParameters) {
    AppConfig cfg = configManager.getConfig();
    
    // Explicitly configure static IP for softAP to ensure stable routing
    IPAddress apIP(192, 168, 4, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(apIP, apIP, subnet);

    // Setup Wi-Fi Modes based on config settings
    if (cfg.staEnabled && strlen(cfg.staSSID) > 0) {
        Serial.print("[WiFi] Client mode enabled. Connecting to STA SSID: ");
        Serial.println(cfg.staSSID);
        
        WiFi.mode(WIFI_AP_STA);
        WiFi.begin(cfg.staSSID, cfg.staPass);
        WiFi.softAP(cfg.wifiSSID, cfg.wifiPass);
    } else {
        Serial.println("[WiFi] Access Point mode only.");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(cfg.wifiSSID, cfg.wifiPass);
    }
    
    Serial.print("[WiFi] Access Point SSID: ");
    Serial.println(cfg.wifiSSID);
    Serial.print("[WiFi] AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // Setup mDNS responder
    if (MDNS.begin("spm")) {
        Serial.println("[mDNS] Responder started. Host: http://spm.local/");
        MDNS.addService("http", "tcp", 80);
    } else {
        Serial.println("[mDNS] Error setting up responder!");
    }

    // Setup Captive Portal DNS redirection
    DNSServer dnsServer;
    dnsServer.start(53, "*", apIP);
    Serial.println("[DNS] Captive Portal DNS Server started redirecting all requests to AP IP.");

    // Setup MQTT Clients
    WiFiClient wifiClient;
    PubSubClient mqttClient(wifiClient);
    
    if (cfg.mqttEnabled && strlen(cfg.mqttServer) > 0) {
        mqttClient.setServer(cfg.mqttServer, cfg.mqttPort);
        Serial.printf("[MQTT] Configured broker %s:%d\n", cfg.mqttServer, cfg.mqttPort);
    }

    // Setup Web Server
    DashboardServer::begin();

    uint32_t lastWsPush = millis();
    uint32_t lastMqttPublish = 0;
    uint32_t lastMqttRetry = 0;
    bool wasConnected = false;

    for (;;) {
        // Process next DNS request (high frequency check for captive portal response)
        dnsServer.processNextRequest();

        // Check WiFi STA connection status change
        bool isConnected = (WiFi.status() == WL_CONNECTED);
        if (isConnected && !wasConnected) {
            Serial.print("[WiFi] Connected to network. STA IP address: ");
            Serial.println(WiFi.localIP());
            wasConnected = true;
        } else if (!isConnected && wasConnected) {
            Serial.println("[WiFi] Lost connection to network.");
            wasConnected = false;
        }

        // MQTT telemetry cycle
        if (cfg.mqttEnabled && isConnected && strlen(cfg.mqttServer) > 0) {
            if (!mqttClient.connected()) {
                uint32_t now = millis();
                if (now - lastMqttRetry > 5000 || lastMqttRetry == 0) {
                    lastMqttRetry = now;
                    String clientId = "SapaMonitor-" + String(random(0xffff), HEX);
                    Serial.print("[MQTT] Connecting to broker...");
                    
                    bool connected = false;
                    if (strlen(cfg.mqttUser) > 0) {
                        connected = mqttClient.connect(clientId.c_str(), cfg.mqttUser, cfg.mqttPass);
                    } else {
                        connected = mqttClient.connect(clientId.c_str());
                    }

                    if (connected) {
                        Serial.println("OK");
                    } else {
                        Serial.printf("FAILED (rc=%d)\n", mqttClient.state());
                    }
                }
            } else {
                mqttClient.loop();

                uint32_t now = millis();
                if (now - lastMqttPublish >= cfg.mqttInterval || lastMqttPublish == 0) {
                    lastMqttPublish = now;
                    
                    // Thread-safe fetch of sensor readings
                    SensorData data = sysState.getData();

                    StaticJsonDocument<384> doc;
                    doc["dcV1"] = data.dcVoltage1;
                    doc["dcA1"] = data.dcCurrent1;
                    doc["dcP1"] = data.dcPower1;
                    
                    doc["dcV2"] = data.dcVoltage2;
                    doc["dcA2"] = data.dcCurrent2;
                    doc["dcP2"] = data.dcPower2;

                    doc["acV1"] = data.acVoltage1;
                    doc["acV2"] = data.acVoltage2;
                    doc["acA"]  = data.acCurrent;

                    doc["rpm"] = data.rpm;
                    doc["t1"]  = data.temperature1;
                    doc["t2"]  = data.temperature2;
                    doc["uptime"] = esp_timer_get_time() / 1000000ULL;

                    char buffer[384];
                    size_t len = serializeJson(doc, buffer);
                    
                    if (mqttClient.publish(cfg.mqttTopic, (uint8_t*)buffer, len, false)) {
                        Serial.printf("[MQTT] Sent telemetry on: %s\n", cfg.mqttTopic);
                    } else {
                        Serial.println("[MQTT] Publish failed!");
                    }
                }
            }
        }

        // Periodic WebSocket push data
        uint32_t now = millis();
        if (now - lastWsPush >= cfg.wsPushMs) {
            DashboardServer::pushData();
            lastWsPush = now;
        }

        // Delay 20ms to allow high-frequency DNS query resolution without hogging Core 0
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
