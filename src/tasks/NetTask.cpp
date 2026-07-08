#include "NetTask.h"
#include "../config.h"
#include "../net/WebServer.h"
#include "../core/ConfigManager.h"
#include <WiFi.h>

void NetTask::start() {
    xTaskCreatePinnedToCore(
        NetTask::taskFunction,
        "NetTask",
        8192,           // Larger stack for network/web server
        NULL,           // Parameters
        1,              // Priority
        NULL,           // Task handle
        0               // Core 0 (Protocol Core)
    );
}

void NetTask::taskFunction(void* pvParameters) {
    // Setup Wi-Fi as Access Point for portable system using config
    WiFi.mode(WIFI_AP);
    WiFi.softAP(configManager.getConfig().wifiSSID, configManager.getConfig().wifiPass);
    
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());

    // Setup Web Server
    DashboardServer::begin();

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(configManager.getConfig().wsPushMs);

    for (;;) {
        // Push updates to all connected websocket clients periodically
        DashboardServer::pushData();

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
