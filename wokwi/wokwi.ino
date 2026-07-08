/* ====================================================================
   Wind Turbine Monitoring System — Wokwi All-in-One Simulation Sketch
   ==================================================================== */

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <INA226.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>

// --- PIN DEFINITIONS ---
#define PIN_ZMPT101B 32  // ADC1_CH4
#define PIN_I2C_SDA  21
#define PIN_I2C_SCL  22
#define PIN_BLDC_HALL 34
#define PIN_DS18B20  4

// ====================================================================
// WEB DASHBOARD ASSETS (Embedded directly in flash for easy simulation)
// ====================================================================

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wind Turbine Monitor</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div id="app">
        <!-- Sidebar Navigation -->
        <nav class="sidebar" id="sidebar">
            <div class="sidebar-brand">
                <svg class="brand-icon" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M12 2C12 2 8 6 8 12s4 10 4 10"/>
                    <path d="M12 2C12 2 16 6 16 12s-4 10-4 10"/>
                    <circle cx="12" cy="12" r="10"/>
                    <line x1="2" y1="12" x2="22" y2="12"/>
                </svg>
                <span class="brand-text">WindMon</span>
            </div>

            <div class="nav-links">
                <button class="nav-btn active" data-page="dashboard" id="nav-dashboard">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <rect x="3" y="3" width="7" height="7" rx="1"/>
                        <rect x="14" y="3" width="7" height="7" rx="1"/>
                        <rect x="3" y="14" width="7" height="7" rx="1"/>
                        <rect x="14" y="14" width="7" height="7" rx="1"/>
                    </svg>
                    <span>Dashboard</span>
                </button>
                <button class="nav-btn" data-page="settings" id="nav-settings">
                    <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                        <circle cx="12" cy="12" r="3"/>
                        <path d="M12 1v2M12 21v2M4.22 4.22l1.42 1.42M18.36 18.36l1.42 1.42M1 12h2M21 12h2M4.22 19.78l1.42-1.42M18.36 5.64l1.42-1.42"/>
                    </svg>
                    <span>Settings</span>
                </button>
            </div>

            <div class="sidebar-footer">
                <div class="conn-status" id="conn-badge">
                    <span class="conn-dot" id="ws-dot"></span>
                    <span id="ws-label">Offline</span>
                </div>
            </div>
        </nav>

        <!-- Mobile Top Bar -->
        <header class="mobile-bar" id="mobile-bar">
            <button class="hamburger" id="hamburger" aria-label="Toggle menu">
                <span></span><span></span><span></span>
            </button>
            <h1 class="mobile-title">WindMon</h1>
            <div class="conn-status mobile-conn" id="conn-badge-mobile">
                <span class="conn-dot" id="ws-dot-mobile"></span>
            </div>
        </header>

        <!-- Main Content Area -->
        <main class="content">

            <!-- ===== Dashboard Page ===== -->
            <section class="page active" id="page-dashboard">
                <div class="page-header">
                    <h2>Live Dashboard</h2>
                    <span class="page-subtitle" id="uptime-label">Uptime: --</span>
                </div>

                <!-- Hero Power Card -->
                <div class="hero-card" id="card-power">
                    <div class="hero-ring">
                        <svg viewBox="0 0 120 120">
                            <circle class="ring-bg" cx="60" cy="60" r="52"/>
                            <circle class="ring-fill" cx="60" cy="60" r="52" id="power-ring"/>
                        </svg>
                        <div class="hero-value">
                            <span id="val-dcpwr">0.00</span>
                            <small>W</small>
                        </div>
                    </div>
                    <div class="hero-meta">
                        <h3>Power Output</h3>
                        <p class="hero-desc">Real-time DC power from the generator</p>
                    </div>
                </div>

                <!-- Metric Cards Grid -->
                <div class="metrics-grid">
                    <div class="metric-card" data-type="voltage">
                        <div class="metric-icon">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M13 2L3 14h9l-1 8 10-12h-9l1-8z"/>
                            </svg>
                        </div>
                        <div class="metric-body">
                            <span class="metric-label">DC Voltage</span>
                            <span class="metric-value"><span id="val-dcvolt">0.00</span> <small>V</small></span>
                        </div>
                        <div class="metric-bar"><div class="metric-bar-fill" id="bar-dcvolt"></div></div>
                    </div>

                    <div class="metric-card" data-type="current">
                        <div class="metric-icon">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M22 12h-4l-3 9L9 3l-3 9H2"/>
                            </svg>
                        </div>
                        <div class="metric-body">
                            <span class="metric-label">DC Current</span>
                            <span class="metric-value"><span id="val-dccur">0.00</span> <small>A</small></span>
                        </div>
                        <div class="metric-bar"><div class="metric-bar-fill" id="bar-dccur"></div></div>
                    </div>

                    <div class="metric-card" data-type="ac">
                        <div class="metric-icon">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M2 12c2-4 4-8 6-8s4 8 6 8 4-8 6-8"/>
                            </svg>
                        </div>
                        <div class="metric-body">
                            <span class="metric-label">AC Voltage (RMS)</span>
                            <span class="metric-value"><span id="val-acvolt">0.0</span> <small>V</small></span>
                        </div>
                        <div class="metric-bar"><div class="metric-bar-fill" id="bar-acvolt"></div></div>
                    </div>

                    <div class="metric-card" data-type="rpm">
                        <div class="metric-icon">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <circle cx="12" cy="12" r="10"/>
                                <polyline points="12 6 12 12 16 14"/>
                            </svg>
                        </div>
                        <div class="metric-body">
                            <span class="metric-label">Rotor Speed</span>
                            <span class="metric-value"><span id="val-rpm">0</span> <small>RPM</small></span>
                        </div>
                        <div class="metric-bar"><div class="metric-bar-fill" id="bar-rpm"></div></div>
                    </div>

                    <div class="metric-card" data-type="temp">
                        <div class="metric-icon">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M14 14.76V3.5a2.5 2.5 0 0 0-5 0v11.26a4.5 4.5 0 1 0 5 0z"/>
                            </svg>
                        </div>
                        <div class="metric-body">
                            <span class="metric-label">Generator Temp</span>
                            <span class="metric-value"><span id="val-temp">0.0</span> <small>&deg;C</small></span>
                        </div>
                        <div class="metric-bar"><div class="metric-bar-fill" id="bar-temp"></div></div>
                    </div>
                </div>
            </section>

            <!-- ===== Settings Page ===== -->
            <section class="page" id="page-settings">
                <div class="page-header">
                    <h2>Settings</h2>
                    <span class="page-subtitle">Device Configuration</span>
                </div>

                <div class="settings-grid">
                    <!-- WiFi Settings -->
                    <div class="settings-card">
                        <div class="settings-card-header">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M5 12.55a11 11 0 0 1 14.08 0"/>
                                <path d="M1.42 9a16 16 0 0 1 21.16 0"/>
                                <path d="M8.53 16.11a6 6 0 0 1 6.95 0"/>
                                <line x1="12" y1="20" x2="12.01" y2="20"/>
                            </svg>
                            <h3>WiFi / Access Point</h3>
                        </div>
                        <div class="settings-card-body">
                            <div class="form-group">
                                <label for="cfg-ssid">SSID (Network Name)</label>
                                <input type="text" id="cfg-ssid" placeholder="WindTurbine_AP" maxlength="32">
                            </div>
                            <div class="form-group">
                                <label for="cfg-pass">Password</label>
                                <div class="input-group">
                                    <input type="password" id="cfg-pass" placeholder="••••••••" maxlength="63">
                                    <button class="input-addon" id="toggle-pass" aria-label="Show password">
                                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                            <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"/>
                                            <circle cx="12" cy="12" r="3"/>
                                        </svg>
                                    </button>
                                </div>
                            </div>
                        </div>
                    </div>

                    <!-- Sensor Settings -->
                    <div class="settings-card">
                        <div class="settings-card-header">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <path d="M22 12h-4l-3 9L9 3l-3 9H2"/>
                            </svg>
                            <h3>Sensor Configuration</h3>
                        </div>
                        <div class="settings-card-body">
                            <div class="form-group">
                                <label for="cfg-poll">Sensor Poll Interval (ms)</label>
                                <input type="number" id="cfg-poll" placeholder="100" min="50" max="5000" step="10">
                                <span class="form-hint">Lower = faster reads, higher CPU use (50–5000)</span>
                            </div>
                            <div class="form-group">
                                <label for="cfg-ws-push">WebSocket Push Interval (ms)</label>
                                <input type="number" id="cfg-ws-push" placeholder="500" min="100" max="10000" step="50">
                                <span class="form-hint">How often the dashboard receives updates (100–10000)</span>
                            </div>
                            <div class="form-group">
                                <label for="cfg-poles">BLDC Motor Poles</label>
                                <input type="number" id="cfg-poles" placeholder="4" min="2" max="50" step="2">
                                <span class="form-hint">Number of magnetic pole pairs on the rotor</span>
                            </div>
                        </div>
                    </div>

                    <!-- Display Settings -->
                    <div class="settings-card">
                        <div class="settings-card-header">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <rect x="2" y="3" width="20" height="14" rx="2" ry="2"/>
                                <line x1="8" y1="21" x2="16" y2="21"/>
                                <line x1="12" y1="17" x2="12" y2="21"/>
                            </svg>
                            <h3>Display &amp; Limits</h3>
                        </div>
                        <div class="settings-card-body">
                            <div class="form-row">
                                <div class="form-group">
                                    <label for="cfg-max-v">Max Voltage (V)</label>
                                    <input type="number" id="cfg-max-v" placeholder="60" min="1" max="500" step="1">
                                </div>
                                <div class="form-group">
                                    <label for="cfg-max-a">Max Current (A)</label>
                                    <input type="number" id="cfg-max-a" placeholder="20" min="0.1" max="100" step="0.1">
                                </div>
                            </div>
                            <div class="form-row">
                                <div class="form-group">
                                    <label for="cfg-max-rpm">Max RPM</label>
                                    <input type="number" id="cfg-max-rpm" placeholder="3000" min="100" max="50000" step="100">
                                </div>
                                <div class="form-group">
                                    <label for="cfg-max-temp">Max Temp (&deg;C)</label>
                                    <input type="number" id="cfg-max-temp" placeholder="100" min="30" max="200" step="5">
                                </div>
                            </div>
                        </div>
                    </div>

                    <!-- System Info -->
                    <div class="settings-card">
                        <div class="settings-card-header">
                            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                <circle cx="12" cy="12" r="10"/>
                                <line x1="12" y1="16" x2="12" y2="12"/>
                                <line x1="12" y1="8" x2="12.01" y2="8"/>
                            </svg>
                            <h3>System Information</h3>
                        </div>
                        <div class="settings-card-body">
                            <div class="info-row">
                                <span class="info-label">Firmware</span>
                                <span class="info-value" id="sys-fw">v1.0.0</span>
                            </div>
                            <div class="info-row">
                                <span class="info-label">Free Heap</span>
                                <span class="info-value" id="sys-heap">-- KB</span>
                            </div>
                            <div class="info-row">
                                <span class="info-label">Uptime</span>
                                <span class="info-value" id="sys-uptime">--</span>
                            </div>
                            <div class="info-row">
                                <span class="info-label">Connected Clients</span>
                                <span class="info-value" id="sys-clients">0</span>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Action Buttons -->
                <div class="settings-actions">
                    <button class="btn btn-primary" id="btn-save-cfg">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M19 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h11l5 5v11a2 2 0 0 1-2 2z"/>
                            <polyline points="17 21 17 13 7 13 7 21"/>
                            <polyline points="7 3 7 8 15 8"/>
                        </svg>
                        Save Configuration
                    </button>
                    <button class="btn btn-outline" id="btn-restart">
                        <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <polyline points="23 4 23 10 17 10"/>
                            <path d="M20.49 15a9 9 0 1 1-2.12-9.36L23 10"/>
                        </svg>
                        Restart Device
                    </button>
                </div>
            </section>

        </main>
    </div>

    <!-- Toast Notification -->
    <div class="toast" id="toast">
        <span id="toast-msg"></span>
    </div>

    <script src="script.js"></script>
</body>
</html>
)rawliteral";

const char STYLE_CSS[] PROGMEM = R"rawliteral(
/* ============================================
   Wind Turbine Monitor — Premium Dark UI
   ============================================ */

:root {
    --bg-deep: #080c14;
    --bg-surface: #0d1321;
    --bg-card: rgba(17, 24, 43, 0.85);
    --bg-card-hover: rgba(22, 31, 55, 0.95);
    --border-subtle: rgba(255, 255, 255, 0.06);
    --border-hover: rgba(255, 255, 255, 0.12);
    --text-primary: #eaf0ff;
    --text-secondary: #7b8db5;
    --text-muted: #4a5a7a;
    --accent: #3b82f6;
    --accent-glow: rgba(59, 130, 246, 0.25);
    --accent-soft: rgba(59, 130, 246, 0.12);
    --green: #22c55e;
    --green-glow: rgba(34, 197, 94, 0.3);
    --amber: #f59e0b;
    --amber-glow: rgba(245, 158, 11, 0.3);
    --red: #ef4444;
    --red-glow: rgba(239, 68, 68, 0.3);
    --cyan: #06b6d4;
    --cyan-glow: rgba(6, 182, 212, 0.25);
    --purple: #a855f7;
    --purple-glow: rgba(168, 85, 247, 0.25);
    --sidebar-w: 230px;
    --radius: 14px;
    --radius-sm: 8px;
    --transition: 0.25s cubic-bezier(0.4, 0, 0.2, 1);
    --font: system-ui, -apple-system, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
}

*, *::before, *::after {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
}

html { font-size: 15px; }

body {
    font-family: var(--font);
    background: var(--bg-deep);
    color: var(--text-primary);
    min-height: 100vh;
    overflow-x: hidden;
    -webkit-font-smoothing: antialiased;
}

body::before {
    content: '';
    position: fixed;
    top: -30%; left: -10%;
    width: 60%; height: 60%;
    background: radial-gradient(circle, rgba(59,130,246,0.06) 0%, transparent 70%);
    pointer-events: none;
    z-index: 0;
}
body::after {
    content: '';
    position: fixed;
    bottom: -20%; right: -10%;
    width: 50%; height: 50%;
    background: radial-gradient(circle, rgba(168,85,247,0.04) 0%, transparent 70%);
    pointer-events: none;
    z-index: 0;
}

#app {
    display: flex;
    min-height: 100vh;
    position: relative;
    z-index: 1;
}

.sidebar {
    width: var(--sidebar-w);
    background: var(--bg-surface);
    border-right: 1px solid var(--border-subtle);
    display: flex;
    flex-direction: column;
    padding: 1.6rem 1rem;
    position: fixed;
    top: 0; left: 0;
    height: 100vh;
    z-index: 100;
    transition: transform var(--transition);
}

.sidebar-brand {
    display: flex;
    align-items: center;
    gap: 0.7rem;
    padding: 0 0.5rem;
    margin-bottom: 2.5rem;
}

.brand-icon {
    width: 28px; height: 28px;
    color: var(--accent);
    filter: drop-shadow(0 0 6px var(--accent-glow));
}

.brand-text {
    font-size: 1.25rem;
    font-weight: 700;
    letter-spacing: -0.5px;
    background: linear-gradient(135deg, var(--text-primary), var(--accent));
    -webkit-background-clip: text;
    background-clip: text;
    -webkit-text-fill-color: transparent;
}

.nav-links {
    flex: 1;
    display: flex;
    flex-direction: column;
    gap: 0.35rem;
}

.nav-btn {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    padding: 0.7rem 0.9rem;
    border: none;
    border-radius: var(--radius-sm);
    background: transparent;
    color: var(--text-secondary);
    font-size: 0.92rem;
    font-weight: 500;
    cursor: pointer;
    transition: all var(--transition);
    font-family: var(--font);
}

.nav-btn svg {
    width: 20px; height: 20px;
    flex-shrink: 0;
    stroke-width: 1.8;
}

.nav-btn:hover {
    background: var(--accent-soft);
    color: var(--text-primary);
}

.nav-btn.active {
    background: var(--accent-soft);
    color: var(--accent);
    font-weight: 600;
    box-shadow: inset 3px 0 0 var(--accent);
}

.sidebar-footer {
    padding-top: 1rem;
    border-top: 1px solid var(--border-subtle);
}

.conn-status {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    padding: 0.5rem 0.6rem;
    font-size: 0.82rem;
    color: var(--text-muted);
    font-weight: 500;
}

.conn-dot {
    width: 8px; height: 8px;
    border-radius: 50%;
    background: var(--red);
    box-shadow: 0 0 8px var(--red-glow);
    transition: all 0.4s ease;
}

.conn-dot.live {
    background: var(--green);
    box-shadow: 0 0 8px var(--green-glow);
    animation: pulse-dot 2s infinite;
}

@keyframes pulse-dot {
    0%, 100% { box-shadow: 0 0 8px var(--green-glow); }
    50% { box-shadow: 0 0 16px var(--green-glow); }
}

.mobile-bar {
    display: none;
    position: fixed;
    top: 0; left: 0; right: 0;
    height: 56px;
    background: var(--bg-surface);
    border-bottom: 1px solid var(--border-subtle);
    align-items: center;
    padding: 0 1rem;
    z-index: 200;
}

.hamburger {
    width: 36px; height: 36px;
    background: none;
    border: none;
    cursor: pointer;
    display: flex;
    flex-direction: column;
    justify-content: center;
    gap: 5px;
    padding: 6px;
}

.hamburger span {
    display: block;
    height: 2px;
    width: 100%;
    background: var(--text-secondary);
    border-radius: 2px;
    transition: all var(--transition);
}

.mobile-title {
    flex: 1;
    text-align: center;
    font-size: 1.1rem;
    font-weight: 700;
    color: var(--text-primary);
}

.mobile-conn .conn-dot {
    width: 10px; height: 10px;
}

.content {
    flex: 1;
    margin-left: var(--sidebar-w);
    padding: 2.5rem 2.5rem 3rem;
    min-height: 100vh;
}

.page {
    display: none;
    animation: fadeUp 0.35s ease-out;
}

.page.active {
    display: block;
}

@keyframes fadeUp {
    from { opacity: 0; transform: translateY(12px); }
    to { opacity: 1; transform: translateY(0); }
}

.page-header {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    margin-bottom: 2rem;
}

.page-header h2 {
    font-size: 1.6rem;
    font-weight: 700;
    letter-spacing: -0.3px;
}

.page-subtitle {
    font-size: 0.85rem;
    color: var(--text-muted);
    font-weight: 500;
}

.hero-card {
    display: flex;
    align-items: center;
    gap: 2rem;
    background: linear-gradient(135deg, var(--bg-card), rgba(59,130,246,0.06));
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius);
    padding: 2rem 2.5rem;
    margin-bottom: 1.8rem;
    position: relative;
    overflow: hidden;
}

.hero-card::after {
    content: '';
    position: absolute;
    top: -50%; right: -20%;
    width: 300px; height: 300px;
    background: radial-gradient(circle, var(--accent-glow), transparent 70%);
    pointer-events: none;
    opacity: 0.3;
}

.hero-ring {
    position: relative;
    width: 130px; height: 130px;
    flex-shrink: 0;
}

.hero-ring svg {
    width: 100%; height: 100%;
    transform: rotate(-90deg);
}

.ring-bg {
    fill: none;
    stroke: var(--border-subtle);
    stroke-width: 6;
}

.ring-fill {
    fill: none;
    stroke: var(--accent);
    stroke-width: 6;
    stroke-linecap: round;
    stroke-dasharray: 326.7;
    stroke-dashoffset: 326.7;
    transition: stroke-dashoffset 0.8s cubic-bezier(0.4, 0, 0.2, 1);
    filter: drop-shadow(0 0 6px var(--accent-glow));
}

.hero-value {
    position: absolute;
    top: 50%; left: 50%;
    transform: translate(-50%, -50%);
    text-align: center;
}

.hero-value span {
    font-size: 1.9rem;
    font-weight: 700;
    font-variant-numeric: tabular-nums;
    color: var(--text-primary);
}

.hero-value small {
    display: block;
    font-size: 0.8rem;
    color: var(--text-muted);
    font-weight: 500;
    margin-top: -2px;
}

.hero-meta h3 {
    font-size: 1.15rem;
    font-weight: 600;
    margin-bottom: 0.3rem;
}

.hero-meta .hero-desc {
    color: var(--text-muted);
    font-size: 0.85rem;
}

.metrics-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
    gap: 1rem;
}

.metric-card {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius);
    padding: 1.3rem 1.4rem 1rem;
    transition: all var(--transition);
    position: relative;
    overflow: hidden;
}

.metric-card::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 2px;
    opacity: 0;
    transition: opacity var(--transition);
}

.metric-card:hover {
    background: var(--bg-card-hover);
    border-color: var(--border-hover);
    transform: translateY(-2px);
}

.metric-card:hover::before { opacity: 1; }

.metric-card[data-type="voltage"]::before { background: linear-gradient(90deg, var(--amber), transparent); }
.metric-card[data-type="current"]::before { background: linear-gradient(90deg, var(--cyan), transparent); }
.metric-card[data-type="ac"]::before { background: linear-gradient(90deg, var(--purple), transparent); }
.metric-card[data-type="rpm"]::before { background: linear-gradient(90deg, var(--green), transparent); }
.metric-card[data-type="temp"]::before { background: linear-gradient(90deg, var(--red), transparent); }

.metric-icon {
    width: 36px; height: 36px;
    border-radius: var(--radius-sm);
    display: flex;
    align-items: center;
    justify-content: center;
    margin-bottom: 0.8rem;
}

.metric-icon svg {
    width: 20px; height: 20px;
}

.metric-card[data-type="voltage"] .metric-icon { background: rgba(245,158,11,0.12); color: var(--amber); }
.metric-card[data-type="current"] .metric-icon { background: rgba(6,182,212,0.12); color: var(--cyan); }
.metric-card[data-type="ac"] .metric-icon { background: rgba(168,85,247,0.12); color: var(--purple); }
.metric-card[data-type="rpm"] .metric-icon { background: rgba(34,197,94,0.12); color: var(--green); }
.metric-card[data-type="temp"] .metric-icon { background: rgba(239,68,68,0.12); color: var(--red); }

.metric-body {
    display: flex;
    justify-content: space-between;
    align-items: baseline;
    margin-bottom: 0.8rem;
}

.metric-label {
    font-size: 0.82rem;
    color: var(--text-secondary);
    font-weight: 500;
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.metric-value {
    font-size: 1.5rem;
    font-weight: 700;
    font-variant-numeric: tabular-nums;
}

.metric-value small {
    font-size: 0.75rem;
    color: var(--text-muted);
    font-weight: 500;
    margin-left: 2px;
}

.metric-bar {
    height: 4px;
    background: var(--border-subtle);
    border-radius: 4px;
    overflow: hidden;
}

.metric-bar-fill {
    height: 100%;
    width: 0%;
    border-radius: 4px;
    transition: width 0.6s cubic-bezier(0.4, 0, 0.2, 1);
}

.metric-card[data-type="voltage"] .metric-bar-fill { background: linear-gradient(90deg, var(--amber), rgba(245,158,11,0.4)); }
.metric-card[data-type="current"] .metric-bar-fill { background: linear-gradient(90deg, var(--cyan), rgba(6,182,212,0.4)); }
.metric-card[data-type="ac"] .metric-bar-fill { background: linear-gradient(90deg, var(--purple), rgba(168,85,247,0.4)); }
.metric-card[data-type="rpm"] .metric-bar-fill { background: linear-gradient(90deg, var(--green), rgba(34,197,94,0.4)); }
.metric-card[data-type="temp"] .metric-bar-fill { background: linear-gradient(90deg, var(--red), rgba(239,68,68,0.4)); }

.settings-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(340px, 1fr));
    gap: 1.2rem;
    margin-bottom: 1.5rem;
}

.settings-card {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius);
    overflow: hidden;
    transition: border-color var(--transition);
}

.settings-card:hover {
    border-color: var(--border-hover);
}

.settings-card-header {
    display: flex;
    align-items: center;
    gap: 0.7rem;
    padding: 1rem 1.3rem;
    background: rgba(255,255,255,0.02);
    border-bottom: 1px solid var(--border-subtle);
}

.settings-card-header svg {
    width: 20px; height: 20px;
    color: var(--accent);
    flex-shrink: 0;
}

.settings-card-header h3 {
    font-size: 0.95rem;
    font-weight: 600;
}

.settings-card-body {
    padding: 1.2rem 1.3rem;
    display: flex;
    flex-direction: column;
    gap: 1rem;
}

.form-group {
    display: flex;
    flex-direction: column;
    gap: 0.3rem;
}

.form-group label {
    font-size: 0.82rem;
    color: var(--text-secondary);
    font-weight: 500;
}

.form-group input {
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-sm);
    padding: 0.6rem 0.85rem;
    color: var(--text-primary);
    font-size: 0.9rem;
    font-family: var(--font);
    transition: all var(--transition);
    outline: none;
}

.form-group input:focus {
    border-color: var(--accent);
    box-shadow: 0 0 0 3px var(--accent-soft);
}

.form-group input::placeholder {
    color: var(--text-muted);
}

.form-hint {
    font-size: 0.75rem;
    color: var(--text-muted);
}

.form-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.8rem;
}

.input-group {
    display: flex;
    gap: 0;
}

.input-group input {
    flex: 1;
    border-top-right-radius: 0;
    border-bottom-right-radius: 0;
}

.input-addon {
    background: var(--bg-surface);
    border: 1px solid var(--border-subtle);
    border-left: none;
    border-radius: 0 var(--radius-sm) var(--radius-sm) 0;
    padding: 0 0.7rem;
    color: var(--text-muted);
    cursor: pointer;
    transition: color var(--transition);
    display: flex;
    align-items: center;
}

.input-addon:hover { color: var(--accent); }
.input-addon svg { width: 18px; height: 18px; }

.info-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.5rem 0;
    border-bottom: 1px solid var(--border-subtle);
}

.info-row:last-child { border-bottom: none; }

.info-label {
    font-size: 0.85rem;
    color: var(--text-secondary);
}

.info-value {
    font-size: 0.85rem;
    font-weight: 600;
    color: var(--text-primary);
    font-variant-numeric: tabular-nums;
}

.settings-actions {
    display: flex;
    gap: 0.8rem;
    flex-wrap: wrap;
}

.btn {
    display: inline-flex;
    align-items: center;
    gap: 0.5rem;
    padding: 0.65rem 1.3rem;
    border-radius: var(--radius-sm);
    font-size: 0.9rem;
    font-weight: 600;
    font-family: var(--font);
    cursor: pointer;
    transition: all var(--transition);
    border: none;
}

.btn svg {
    width: 18px; height: 18px;
}

.btn-primary {
    background: var(--accent);
    color: white;
    box-shadow: 0 2px 12px var(--accent-glow);
}

.btn-primary:hover {
    background: #2563eb;
    box-shadow: 0 4px 20px var(--accent-glow);
    transform: translateY(-1px);
}

.btn-primary:active { transform: translateY(0); }

.btn-outline {
    background: transparent;
    color: var(--text-secondary);
    border: 1px solid var(--border-subtle);
}

.btn-outline:hover {
    border-color: var(--border-hover);
    color: var(--text-primary);
    background: rgba(255,255,255,0.03);
}

.toast {
    position: fixed;
    bottom: 2rem;
    left: 50%;
    transform: translateX(-50%) translateY(100px);
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-sm);
    padding: 0.7rem 1.4rem;
    font-size: 0.88rem;
    font-weight: 500;
    color: var(--text-primary);
    box-shadow: 0 8px 30px rgba(0,0,0,0.4);
    backdrop-filter: blur(12px);
    z-index: 999;
    opacity: 0;
    transition: all 0.4s cubic-bezier(0.4, 0, 0.2, 1);
    pointer-events: none;
}

.toast.show {
    opacity: 1;
    transform: translateX(-50%) translateY(0);
    pointer-events: auto;
}

.toast.success { border-color: var(--green); }
.toast.error { border-color: var(--red); }

::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-track { background: transparent; }
::-webkit-scrollbar-thumb {
    background: rgba(255,255,255,0.1);
    border-radius: 3px;
}
::-webkit-scrollbar-thumb:hover { background: rgba(255,255,255,0.2); }

@media (max-width: 768px) {
    .sidebar {
        transform: translateX(-100%);
    }
    .sidebar.open {
        transform: translateX(0);
        box-shadow: 10px 0 40px rgba(0,0,0,0.5);
    }
    .mobile-bar {
        display: flex;
    }
    .content {
        margin-left: 0;
        padding: 72px 1rem 2rem;
    }
    .hero-card {
        flex-direction: column;
        text-align: center;
        padding: 1.5rem;
    }
    .metrics-grid {
        grid-template-columns: 1fr 1fr;
    }
    .settings-grid {
        grid-template-columns: 1fr;
    }
    .form-row {
        grid-template-columns: 1fr;
    }
}

@media (max-width: 480px) {
    .metrics-grid {
        grid-template-columns: 1fr;
    }
}
)rawliteral";

const char SCRIPT_JS[] PROGMEM = R"rawliteral(
/* ============================================
   Wind Turbine Monitor — Application Logic
   ============================================ */

(function () {
    'use strict';

    const DEFAULT_CFG = {
        maxVoltage: 60,
        maxCurrent: 20,
        maxRPM: 3000,
        maxTemp: 100
    };

    let cfg = Object.assign({}, DEFAULT_CFG);
    const gateway = `ws://${window.location.hostname}/ws`;
    let ws = null;
    let wsReconnectTimer = null;
    const $ = (id) => document.getElementById(id);

    const dom = {
        dcPwr:   $('val-dcpwr'),
        dcVolt:  $('val-dcvolt'),
        dcCur:   $('val-dccur'),
        acVolt:  $('val-acvolt'),
        rpm:     $('val-rpm'),
        temp:    $('val-temp'),
        barDcVolt: $('bar-dcvolt'),
        barDcCur:  $('bar-dccur'),
        barAcVolt: $('bar-acvolt'),
        barRpm:    $('bar-rpm'),
        barTemp:   $('bar-temp'),
        powerRing: $('power-ring'),
        wsDot:       $('ws-dot'),
        wsLabel:     $('ws-label'),
        wsDotMobile: $('ws-dot-mobile'),
        uptimeLabel: $('uptime-label'),
        navDashboard: $('nav-dashboard'),
        navSettings:  $('nav-settings'),
        pageDashboard: $('page-dashboard'),
        pageSettings:  $('page-settings'),
        hamburger: $('hamburger'),
        sidebar:   $('sidebar'),
        cfgSsid:   $('cfg-ssid'),
        cfgPass:   $('cfg-pass'),
        cfgPoll:   $('cfg-poll'),
        cfgWsPush: $('cfg-ws-push'),
        cfgPoles:  $('cfg-poles'),
        cfgMaxV:   $('cfg-max-v'),
        cfgMaxA:   $('cfg-max-a'),
        cfgMaxRpm: $('cfg-max-rpm'),
        cfgMaxTemp:$('cfg-max-temp'),
        btnSave:    $('btn-save-cfg'),
        btnRestart: $('btn-restart'),
        togglePass: $('toggle-pass'),
        sysFw:      $('sys-fw'),
        sysHeap:    $('sys-heap'),
        sysUptime:  $('sys-uptime'),
        sysClients: $('sys-clients'),
        toast:    $('toast'),
        toastMsg: $('toast-msg')
    };

    function switchPage(page) {
        document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
        document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
        const target = $('page-' + page);
        const btn = $('nav-' + page);
        if (target) target.classList.add('active');
        if (btn) btn.classList.add('active');
        dom.sidebar.classList.remove('open');
        if (page === 'settings') {
            loadConfig();
            loadSysInfo();
        }
    }

    dom.navDashboard.addEventListener('click', () => switchPage('dashboard'));
    dom.navSettings.addEventListener('click', () => switchPage('settings'));

    dom.hamburger.addEventListener('click', () => {
        dom.sidebar.classList.toggle('open');
    });

    document.addEventListener('click', (e) => {
        if (window.innerWidth <= 768 &&
            dom.sidebar.classList.contains('open') &&
            !dom.sidebar.contains(e.target) &&
            !dom.hamburger.contains(e.target)) {
            dom.sidebar.classList.remove('open');
        }
    });

    dom.togglePass.addEventListener('click', () => {
        const inp = dom.cfgPass;
        inp.type = inp.type === 'password' ? 'text' : 'password';
    });

    let toastTimer = null;
    function showToast(msg, type) {
        dom.toastMsg.textContent = msg;
        dom.toast.className = 'toast show ' + (type || '');
        clearTimeout(toastTimer);
        toastTimer = setTimeout(() => {
            dom.toast.className = 'toast';
        }, 3000);
    }

    function connectWS() {
        if (ws && ws.readyState === WebSocket.OPEN) return;
        ws = new WebSocket(gateway);
        ws.onopen = () => {
            setConnectionStatus(true);
            clearTimeout(wsReconnectTimer);
        };
        ws.onclose = () => {
            setConnectionStatus(false);
            wsReconnectTimer = setTimeout(connectWS, 2000);
        };
        ws.onerror = () => {
            ws.close();
        };
        ws.onmessage = (event) => {
            try {
                const data = JSON.parse(event.data);
                updateDashboard(data);
            } catch (e) {
                console.error('JSON parse error:', e);
            }
        };
    }

    function setConnectionStatus(connected) {
        const dots = [dom.wsDot, dom.wsDotMobile];
        dots.forEach(dot => {
            if (dot) {
                dot.classList.toggle('live', connected);
            }
        });
        if (dom.wsLabel) {
            dom.wsLabel.textContent = connected ? 'Live' : 'Offline';
        }
    }

    function updateDashboard(data) {
        setText(dom.dcPwr, data.dcPwr != null ? data.dcPwr.toFixed(2) : '0.00');
        setText(dom.dcVolt, data.dcVolt != null ? data.dcVolt.toFixed(2) : '0.00');
        setText(dom.dcCur, data.dcCur != null ? data.dcCur.toFixed(2) : '0.00');
        setText(dom.acVolt, data.acVolt != null ? data.acVolt.toFixed(1) : '0.0');
        setText(dom.rpm, data.rpm != null ? Math.round(data.rpm).toString() : '0');
        setText(dom.temp, data.temp != null ? data.temp.toFixed(1) : '0.0');

        setBar(dom.barDcVolt, data.dcVolt, cfg.maxVoltage);
        setBar(dom.barDcCur, data.dcCur, cfg.maxCurrent);
        setBar(dom.barAcVolt, data.acVolt, cfg.maxVoltage);
        setBar(dom.barRpm, data.rpm, cfg.maxRPM);
        setBar(dom.barTemp, data.temp, cfg.maxTemp);

        updatePowerRing(data.dcPwr || 0);

        if (data.uptime != null) {
            setText(dom.uptimeLabel, 'Uptime: ' + formatUptime(data.uptime));
        }
    }

    function setText(el, val) {
        if (el && el.textContent !== val) {
            el.textContent = val;
        }
    }

    function setBar(el, value, max) {
        if (!el) return;
        const pct = Math.min(100, Math.max(0, ((value || 0) / max) * 100));
        el.style.width = pct + '%';
    }

    function updatePowerRing(power) {
        if (!dom.powerRing) return;
        const maxPower = cfg.maxVoltage * cfg.maxCurrent;
        const pct = Math.min(1, Math.max(0, power / maxPower));
        const circumference = 326.7;
        dom.powerRing.style.strokeDashoffset = circumference * (1 - pct);
    }

    function formatUptime(seconds) {
        const d = Math.floor(seconds / 86400);
        const h = Math.floor((seconds % 86400) / 3600);
        const m = Math.floor((seconds % 3600) / 60);
        const s = seconds % 60;
        if (d > 0) return d + 'd ' + h + 'h ' + m + 'm';
        if (h > 0) return h + 'h ' + m + 'm ' + s + 's';
        return m + 'm ' + s + 's';
    }

    function loadConfig() {
        fetch('/api/config')
            .then(r => r.json())
            .then(data => {
                if (data.ssid != null)    dom.cfgSsid.value = data.ssid;
                if (data.pass != null)    dom.cfgPass.value = data.pass;
                if (data.pollMs != null)  dom.cfgPoll.value = data.pollMs;
                if (data.wsPushMs != null) dom.cfgWsPush.value = data.wsPushMs;
                if (data.poles != null)   dom.cfgPoles.value = data.poles;
                if (data.maxV != null)    { dom.cfgMaxV.value = data.maxV; cfg.maxVoltage = data.maxV; }
                if (data.maxA != null)    { dom.cfgMaxA.value = data.maxA; cfg.maxCurrent = data.maxA; }
                if (data.maxRPM != null)  { dom.cfgMaxRpm.value = data.maxRPM; cfg.maxRPM = data.maxRPM; }
                if (data.maxTemp != null) { dom.cfgMaxTemp.value = data.maxTemp; cfg.maxTemp = data.maxTemp; }
            })
            .catch(() => {});
    }

    function loadSysInfo() {
        fetch('/api/sysinfo')
            .then(r => r.json())
            .then(data => {
                if (data.fw != null)      dom.sysFw.textContent = data.fw;
                if (data.heap != null)    dom.sysHeap.textContent = (data.heap / 1024).toFixed(1) + ' KB';
                if (data.uptime != null)  dom.sysUptime.textContent = formatUptime(data.uptime);
                if (data.clients != null) dom.sysClients.textContent = data.clients;
            })
            .catch(() => {});
    }

    dom.btnSave.addEventListener('click', () => {
        const payload = {};
        if (dom.cfgSsid.value)    payload.ssid = dom.cfgSsid.value;
        if (dom.cfgPass.value)    payload.pass = dom.cfgPass.value;
        if (dom.cfgPoll.value)    payload.pollMs = parseInt(dom.cfgPoll.value, 10);
        if (dom.cfgWsPush.value)  payload.wsPushMs = parseInt(dom.cfgWsPush.value, 10);
        if (dom.cfgPoles.value)   payload.poles = parseInt(dom.cfgPoles.value, 10);
        if (dom.cfgMaxV.value)    payload.maxV = parseFloat(dom.cfgMaxV.value);
        if (dom.cfgMaxA.value)    payload.maxA = parseFloat(dom.cfgMaxA.value);
        if (dom.cfgMaxRpm.value)  payload.maxRPM = parseInt(dom.cfgMaxRpm.value, 10);
        if (dom.cfgMaxTemp.value) payload.maxTemp = parseInt(dom.cfgMaxTemp.value, 10);

        fetch('/api/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        })
        .then(r => r.json())
        .then(data => {
            if (data.ok) {
                showToast('Configuration saved successfully!', 'success');
                if (payload.maxV)    cfg.maxVoltage = payload.maxV;
                if (payload.maxA)    cfg.maxCurrent = payload.maxA;
                if (payload.maxRPM)  cfg.maxRPM = payload.maxRPM;
                if (payload.maxTemp) cfg.maxTemp = payload.maxTemp;
            } else {
                showToast('Failed to save: ' + (data.error || 'Unknown error'), 'error');
            }
        })
        .catch(() => {
            showToast('Connection error — could not save', 'error');
        });
    });

    dom.btnRestart.addEventListener('click', () => {
        if (!confirm('Restart the device? You will be disconnected briefly.')) return;
        fetch('/api/restart', { method: 'POST' })
            .then(() => {
                showToast('Device restarting...', 'success');
                setTimeout(() => { window.location.reload(); }, 5000);
            })
            .catch(() => {
                showToast('Restart command sent', 'success');
                setTimeout(() => { window.location.reload(); }, 5000);
            });
    });

    connectWS();
    loadConfig();

})();
)rawliteral";

// ====================================================================
// SENSOR CLASSES & HARDWARE DRIVERS (With fallback simulation logic)
// ====================================================================

// --- 1. ZMPT101B AC Voltage Sensor ---
class ZMPT101B {
public:
    ZMPT101B(uint8_t pin) : _pin(pin), _calibrationFactor(1.0f) {}
    void begin() {
        pinMode(_pin, INPUT);
    }
    float readACVoltage() {
        // Read raw values
        uint32_t sum = 0;
        for (int i = 0; i < 100; i++) {
            sum += analogRead(_pin);
            delayMicroseconds(50); 
        }
        float avgRaw = sum / 100.0f;

        // Fallback simulation if input pin reads 0 or floating (highly typical in simulation)
        if (avgRaw < 10.0f) {
            // Generate a realistic 220V RMS sine wave variation
            return 220.0f + 12.0f * sin(millis() / 4000.0f);
        }

        // Real mapping logic for ADC1
        float voltagePeak = (avgRaw / 4095.0f) * 3.3f * 350.0f; // Scale to mains AC range
        float rms = (voltagePeak / 1.414f) * _calibrationFactor;
        return rms;
    }
private:
    uint8_t _pin;
    float _calibrationFactor;
};

// --- 2. INA226 DC Voltage/Current/Power Sensor ---
class INA226Sensor {
public:
    INA226Sensor(uint8_t address, uint8_t sda, uint8_t scl) 
        : _ina(address), _address(address), _sda(sda), _scl(scl), _connected(false) {}
    
    bool begin() {
        Wire.begin(_sda, _scl);
        _connected = _ina.begin();
        if (_connected) {
            // Configure INA226 (Shunt resistance = 0.002 Ohm, Max Current = 20A)
            _ina.setMaxCurrentShunt(20.0f, 0.002f);
        } else {
            Serial.println("[Simulation] INA226 not found. Running simulated DC metrics.");
        }
        return _connected;
    }

    bool isConnected() const { return _connected; }
    
    float getVoltage() {
        if (!_connected) {
            // Simulated DC battery voltage cycling around 24V
            return 24.0f + 1.8f * sin(millis() / 5000.0f);
        }
        return _ina.getBusVoltage();
    }
    
    float getCurrent() {
        if (!_connected) {
            // Simulated DC current cycling around 5A
            float cur = 5.0f + 3.2f * sin(millis() / 3000.0f);
            return cur > 0.0f ? cur : 0.0f;
        }
        return _ina.getCurrent();
    }
    
    float getPower() {
        if (!_connected) {
            return getVoltage() * getCurrent();
        }
        return _ina.getPower();
    }

private:
    INA226 _ina;
    uint8_t _address;
    uint8_t _sda;
    uint8_t _scl;
    bool _connected;
};

// --- 3. BLDC Hall RPM Sensor ---
class BLDCHall {
public:
    BLDCHall(uint8_t pin, uint8_t poles) : _pin(pin), _poles(poles) {
        _lastCalculateTime = 0;
        _lastPulseCount = 0;
    }
    
    void begin() {
        pinMode(_pin, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(_pin), handleISR, RISING);
    }
    
    void setPoles(uint8_t poles) { _poles = poles; }
    
    float getRPM() {
        uint32_t currentMillis = millis();
        uint32_t currentPulses = pulseCount;
        uint32_t timeDelta = currentMillis - _lastCalculateTime;
        
        if (timeDelta < 10) return 0.0f;

        uint32_t pulses = currentPulses - _lastPulseCount;
        float rpm = ((float)pulses / timeDelta) * 1000.0f * 60.0f / (float)_poles;

        _lastCalculateTime = currentMillis;
        _lastPulseCount = currentPulses;

        // Fallback simulation: if rotor speed is static in simulation, auto-simulate it
        if (currentMillis - lastPulseTime > 1500) {
            // Mock speed cycling between 800 and 1500 RPM
            rpm = 1100.0f + 350.0f * sin(millis() / 6000.0f);
        }
        return rpm;
    }

    static void IRAM_ATTR handleISR() {
        pulseCount++;
        lastPulseTime = millis();
    }

    static volatile uint32_t pulseCount;
    static volatile uint32_t lastPulseTime;

private:
    uint8_t _pin;
    uint8_t _poles;
    uint32_t _lastCalculateTime;
    uint32_t _lastPulseCount;
};

volatile uint32_t BLDCHall::pulseCount = 0;
volatile uint32_t BLDCHall::lastPulseTime = 0;

void IRAM_ATTR handleISR() {
    BLDCHall::handleISR();
}

// --- 4. DS18B20 1-Wire Temperature Sensor ---
class TemperatureSensor {
public:
    TemperatureSensor(uint8_t pin) : _pin(pin), _oneWire(pin), _sensors(&_oneWire) {}
    void begin() {
        _sensors.begin();
        _sensors.setWaitForConversion(false); // Non-blocking reads
    }
    void requestTemperature() {
        _sensors.requestTemperatures();
    }
    float getTemperature() {
        float temp = _sensors.getTempCByIndex(0);
        // Fallback simulation if no DS18B20 detected
        if (temp < -100.0f || temp > 150.0f) {
            // Simulated generator temperature cycle
            return 38.5f + 4.2f * sin(millis() / 8000.0f);
        }
        return temp;
    }
private:
    uint8_t _pin;
    OneWire _oneWire;
    DallasTemperature _sensors;
};

// ====================================================================
// CORE STATE & CONFIGURATION STORAGE (Preferences / NVS)
// ====================================================================

struct SensorData {
    float acVoltage;
    float dcVoltage;
    float dcCurrent;
    float dcPower;
    float rpm;
    float temperatureC;
};

class SystemState {
public:
    SystemState() {
        _mutex = xSemaphoreCreateMutex();
        _data = {0};
    }
    
    SensorData getData() {
        SensorData copy;
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            copy = _data;
            xSemaphoreGive(_mutex);
        }
        return copy;
    }
    
    void update(float acV, float dcV, float dcA, float dcW, float rpmVal, float tempVal) {
        if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
            _data.acVoltage = acV;
            _data.dcVoltage = dcV;
            _data.dcCurrent = dcA;
            _data.dcPower = dcW;
            _data.rpm = rpmVal;
            _data.temperatureC = tempVal;
            xSemaphoreGive(_mutex);
        }
    }
private:
    SensorData _data;
    SemaphoreHandle_t _mutex;
};

SystemState sysState;

struct AppConfig {
    char wifiSSID[33];
    char wifiPass[64];
    uint32_t sensorPollMs;
    uint32_t wsPushMs;
    uint32_t bldcPoles;
    float maxV;
    float maxA;
    uint32_t maxRPM;
    uint32_t maxTemp;
};

class ConfigManager {
public:
    ConfigManager() {
        // Safe factory defaults
        strncpy(_config.wifiSSID, "Wokwi-GUEST", 32); // Ready for Wokwi internet out of the box!
        _config.wifiPass[0] = '\0';
        _config.sensorPollMs = 100;
        _config.wsPushMs = 500;
        _config.bldcPoles = 4;
        _config.maxV = 60.0f;
        _config.maxA = 20.0f;
        _config.maxRPM = 3000;
        _config.maxTemp = 100;
    }
    
    void begin() {
        _prefs.begin("windmon", false);
        
        String ssid = _prefs.getString("ssid", _config.wifiSSID);
        String pass = _prefs.getString("pass", _config.wifiPass);
        
        strncpy(_config.wifiSSID, ssid.c_str(), sizeof(_config.wifiSSID) - 1);
        strncpy(_config.wifiPass, pass.c_str(), sizeof(_config.wifiPass) - 1);
        
        _config.sensorPollMs = _prefs.getUInt("pollMs", _config.sensorPollMs);
        _config.wsPushMs = _prefs.getUInt("wsPushMs", _config.wsPushMs);
        _config.bldcPoles = _prefs.getUInt("poles", _config.bldcPoles);
        _config.maxV = _prefs.getFloat("maxV", _config.maxV);
        _config.maxA = _prefs.getFloat("maxA", _config.maxA);
        _config.maxRPM = _prefs.getUInt("maxRPM", _config.maxRPM);
        _config.maxTemp = _prefs.getUInt("maxTemp", _config.maxTemp);
        
        _prefs.end();
    }
    
    void save() {
        _prefs.begin("windmon", false);
        _prefs.putString("ssid", _config.wifiSSID);
        _prefs.putString("pass", _config.wifiPass);
        _prefs.putUInt("pollMs", _config.sensorPollMs);
        _prefs.putUInt("wsPushMs", _config.wsPushMs);
        _prefs.putUInt("poles", _config.bldcPoles);
        _prefs.putFloat("maxV", _config.maxV);
        _prefs.putFloat("maxA", _config.maxA);
        _prefs.putUInt("maxRPM", _config.maxRPM);
        _prefs.putUInt("maxTemp", _config.maxTemp);
        _prefs.end();
    }
    
    const AppConfig& getConfig() const { return _config; }
    
    void updateFromJson(const JsonVariant& json) {
        if (json["ssid"].is<const char*>()) strncpy(_config.wifiSSID, json["ssid"], 32);
        if (json["pass"].is<const char*>()) strncpy(_config.wifiPass, json["pass"], 63);
        if (json["pollMs"].is<uint32_t>()) _config.sensorPollMs = json["pollMs"];
        if (json["wsPushMs"].is<uint32_t>()) _config.wsPushMs = json["wsPushMs"];
        if (json["poles"].is<uint32_t>()) _config.bldcPoles = json["poles"];
        if (json["maxV"].is<float>()) _config.maxV = json["maxV"];
        if (json["maxA"].is<float>()) _config.maxA = json["maxA"];
        if (json["maxRPM"].is<uint32_t>()) _config.maxRPM = json["maxRPM"];
        if (json["maxTemp"].is<uint32_t>()) _config.maxTemp = json["maxTemp"];
    }
    
    void serialize(JsonDocument& doc) const {
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

private:
    AppConfig _config;
    Preferences _prefs;
};

ConfigManager configManager;

// ====================================================================
// ASYNC WEB SERVER & WEBSOCKET MODULES
// ====================================================================

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
        Serial.printf("WebSocket client #%u connected\n", client->id());
    } else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("WebSocket client #%u disconnected\n", client->id());
    }
}

class DashboardServer {
public:
    static void begin() {
        // Serve Web Assets from RAM
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send_P(200, "text/html", INDEX_HTML);
        });
        server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send_P(200, "text/css", STYLE_CSS);
        });
        server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
            request->send_P(200, "text/javascript", SCRIPT_JS);
        });

        // Config Endpoints
        server.on("/api/config", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<512> doc;
            configManager.serialize(doc);
            serializeJson(doc, *response);
            request->send(response);
        });

        // Save Config Handler
        AsyncCallbackJsonWebHandler* configHandler = new AsyncCallbackJsonWebHandler("/api/config", [](AsyncWebServerRequest *request, JsonVariant &json) {
            configManager.updateFromJson(json);
            configManager.save();
            request->send(200, "application/json", "{\"ok\":true}");
        });
        server.addHandler(configHandler);

        // System Health Status
        server.on("/api/sysinfo", HTTP_GET, [](AsyncWebServerRequest *request){
            AsyncResponseStream *response = request->beginResponseStream("application/json");
            StaticJsonDocument<256> doc;
            doc["fw"] = "v1.0.0-simulation";
            doc["heap"] = ESP.getFreeHeap();
            doc["uptime"] = esp_timer_get_time() / 1000000ULL;
            doc["clients"] = ws.count();
            serializeJson(doc, *response);
            request->send(response);
        });

        // Device Reboot
        server.on("/api/restart", HTTP_POST, [](AsyncWebServerRequest *request){
            request->send(200, "application/json", "{\"ok\":true}");
            xTaskCreate([](void*){
                vTaskDelay(pdMS_TO_TICKS(1000));
                ESP.restart();
            }, "reboot_task", 1024, NULL, 1, NULL);
        });

        ws.onEvent(onWsEvent);
        server.addHandler(&ws);
        server.begin();
    }

    static void pushData() {
        ws.cleanupClients();
        if (ws.count() > 0) {
            SensorData data = sysState.getData();
            StaticJsonDocument<256> doc;
            doc["acVolt"] = data.acVoltage;
            doc["dcVolt"] = data.dcVoltage;
            doc["dcCur"] = data.dcCurrent;
            doc["dcPwr"] = data.dcPower;
            doc["rpm"] = data.rpm;
            doc["temp"] = data.temperatureC;
            doc["uptime"] = esp_timer_get_time() / 1000000ULL;

            char buffer[256];
            size_t len = serializeJson(doc, buffer);
            ws.textAll(buffer, len);
        }
    }
};

// ====================================================================
// FREERTOS TASK LOGIC (Pinned to appropriate cores)
// ====================================================================

// --- Core 1 Task: High Frequency Sensor Scanning ---
class SensorTask {
public:
    static void start() {
        xTaskCreatePinnedToCore(taskFunction, "SensorTask", 4096, NULL, 1, NULL, 1);
    }
private:
    static void taskFunction(void* pvParameters) {
        // Instantiate hardware drivers
        ZMPT101B zmpt(PIN_ZMPT101B);
        INA226Sensor ina(0x40, PIN_I2C_SDA, PIN_I2C_SCL);
        BLDCHall hall(PIN_BLDC_HALL, configManager.getConfig().bldcPoles);
        TemperatureSensor temp(PIN_DS18B20);

        // Hardware initialization
        zmpt.begin();
        ina.begin();
        hall.begin();
        temp.begin();

        TickType_t xLastWakeTime = xTaskGetTickCount();
        
        for (;;) {
            // Apply pole config updates on the fly
            hall.setPoles(configManager.getConfig().bldcPoles);

            // Read sensor metrics
            float acV = zmpt.readACVoltage();
            float dcV = ina.getVoltage();
            float dcA = ina.getCurrent();
            float dcW = ina.getPower();
            float rpm = hall.getRPM();
            
            temp.requestTemperature();
            float tC = temp.getTemperature();

            // Thread-safe update of the global system state
            sysState.update(acV, dcV, dcA, dcW, rpm, tC);

            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(configManager.getConfig().sensorPollMs));
        }
    }
};

// --- Core 0 Task: Network Management & WS Broadcasting ---
class NetTask {
public:
    static void start() {
        xTaskCreatePinnedToCore(taskFunction, "NetTask", 8192, NULL, 1, NULL, 0);
    }
private:
    static void taskFunction(void* pvParameters) {
        AppConfig cfg = configManager.getConfig();

        // Connect to local station if SSID is "Wokwi-GUEST", otherwise host Access Point
        if (strcmp(cfg.wifiSSID, "Wokwi-GUEST") == 0) {
            Serial.print("Connecting to Wokwi WiFi...");
            WiFi.mode(WIFI_STA);
            WiFi.begin("Wokwi-GUEST", "");
            while (WiFi.status() != WL_CONNECTED) {
                vTaskDelay(pdMS_TO_TICKS(500));
                Serial.print(".");
            }
            Serial.println("\n[WiFi] Connected successfully to simulator gateway!");
            Serial.print("[WiFi] IP Address: ");
            Serial.println(WiFi.localIP());
        } else {
            Serial.print("Starting AP SSID: ");
            Serial.println(cfg.wifiSSID);
            WiFi.mode(WIFI_AP);
            WiFi.softAP(cfg.wifiSSID, cfg.wifiPass);
            Serial.print("[WiFi] AP IP Address: ");
            Serial.println(WiFi.softAPIP());
        }

        // Start server routes
        DashboardServer::begin();

        TickType_t xLastWakeTime = xTaskGetTickCount();
        
        for (;;) {
            // Push WebSocket updates to dashboard clients
            DashboardServer::pushData();
            vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(configManager.getConfig().wsPushMs));
        }
    }
};

// ====================================================================
// ESP32 MAIN ENTRY POINTS
// ====================================================================

void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Wind Turbine Monitor [Wokwi Simulation Mode] ===");

    // Initialize configuration database
    configManager.begin();

    // Start FreeRTOS tasks pinned to separate cores
    SensorTask::start();
    NetTask::start();

    // Self-delete setup task as we've spawned our worker tasks
    vTaskDelete(NULL);
}

void loop() {
    // Idle
}
