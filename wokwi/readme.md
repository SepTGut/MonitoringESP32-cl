# Wind Turbine Monitor Wokwi Simulation

This folder contains a unified, single-file edition of the modular ESP32 wind turbine monitoring system firmware. It is optimized to run directly on the **[Wokwi Online ESP32 Simulator](https://wokwi.com/)** without requiring any filesystem uploads.

## How it works

1. **Embedded Assets:** All redesigned HTML, CSS, and JS code are stored in the program's flash memory (`PROGMEM`) as raw string constants.
2. **NVS Settings:** Settings are saved using the ESP32 `Preferences` library (NVS) instead of LittleFS file writes.
3. **Simulated Sensor Signals:** If the simulator does not detect real physical sensors (such as the INA226 on the I2C bus), it will automatically generate smooth sinusoidal telemetry waves so the dashboard graphs and metrics animate realistically.
4. **Wokwi WiFi STA Auto-mode:** If the configured SSID is `"Wokwi-GUEST"`, the ESP32 connects in STA mode. Wokwi maps this connection to the simulator's NAT gateway, allowing you to access the web panel directly.

## Instructions to run on Wokwi

1. Open [Wokwi.com](https://wokwi.com/) and create a new **ESP32** project.
2. Copy the entire contents of [wokwi.ino](file:///d:/MyCode/MonitoringESP32-cl/wokwi/wokwi.ino) and paste it into the main sketch file in the Wokwi editor.
3. Click the **Library Manager** tab in Wokwi (the library icon on the left sidebar) and add the following dependencies:
   - `ArduinoJson`
   - `ESPAsyncWebServer`
   - `AsyncTCP`
   - `OneWire`
   - `DallasTemperature`
   - `INA226`
4. Start the simulation.
5. In the serial terminal, you will see output like:
   ```
   === Wind Turbine Monitor [Wokwi Simulation Mode] ===
   Connecting to Wokwi WiFi.........
   [WiFi] Connected successfully to simulator gateway!
   [WiFi] IP Address: 10.10.0.18
   ```
6. Wokwi will display a clickable link in the console (or hover popup) pointing to the local web server. Click it to open your premium redesigned dashboard in a new tab!
