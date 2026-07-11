#include "WebServer.h"
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "../core/SystemState.h"
#include "../core/ConfigManager.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
}

void DashboardServer::begin() {
    // Serve static files
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if (LittleFS.exists("/index.html")) {
            request->send(LittleFS, "/index.html", "text/html");
        } else {
            request->send(200, "text/html", "<h1>Monitor SaPa</h1><p>Error: index.html not found in LittleFS. Please upload filesystem assets using <code>pio run -t uploadfs</code>.</p>");
        }
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        if (LittleFS.exists("/style.css")) {
            request->send(LittleFS, "/style.css", "text/css");
        } else {
            request->send(404, "text/plain", "style.css not found");
        }
    });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        if (LittleFS.exists("/script.js")) {
            request->send(LittleFS, "/script.js", "text/javascript");
        } else {
            request->send(404, "text/plain", "script.js not found");
        }
    });

    // --- API Endpoints ---
    
    // Get current config
    server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<1024> doc;
        configManager.serialize(doc);
        serializeJson(doc, *response);
        request->send(response);
    });

    // Save config (JSON POST body)
    AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler("/api/config", [](AsyncWebServerRequest *request, JsonVariant &json) {
        configManager.updateFromJson(json);
        bool success = configManager.save();
        
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<128> doc;
        if (success) {
            doc["ok"] = true;
        } else {
            doc["ok"] = false;
            doc["error"] = "Failed to save configuration";
        }
        serializeJson(doc, *response);
        request->send(response);
    });
    server.addHandler(configHandler);

    // Get system status/info
    server.on("/api/sysinfo", HTTP_GET, [](AsyncWebServerRequest *request){
        AsyncResponseStream *response = request->beginResponseStream("application/json");
        StaticJsonDocument<256> doc;
        doc["fw"] = "v1.0.0";
        doc["heap"] = ESP.getFreeHeap();
        doc["uptime"] = esp_timer_get_time() / 1000000ULL;
        doc["clients"] = ws.count();
        serializeJson(doc, *response);
        request->send(response);
    });

    // Reboot ESP32
    server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "application/json", "{\"ok\":true}");
        xTaskCreate([](void*){
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP.restart();
        }, "restart_task", 2048, NULL, 1, NULL);
    });

    // Handle WebSocket
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    // Captive Portal Redirect Handler
    server.onNotFound([](AsyncWebServerRequest *request){
        String host = request->host();
        if (host != "192.168.4.1" && host != "spm.local" && !host.endsWith(".local")) {
            request->redirect("http://spm.local/");
        } else {
            if (LittleFS.exists("/index.html")) {
                request->send(LittleFS, "/index.html", "text/html");
            } else {
                request->send(200, "text/html", "<h1>Monitor SaPa</h1><p>Error: index.html not found in LittleFS. Please upload filesystem assets using <code>pio run -t uploadfs</code>.</p>");
            }
        }
    });

    server.begin();
}

void DashboardServer::pushData() {
    ws.cleanupClients();
    if (ws.count() > 0) {
        // Get thread-safe copy of sensor data
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
        ws.textAll(buffer, len);
    }
}
