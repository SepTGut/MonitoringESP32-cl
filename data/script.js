/* ============================================
   Monitor SaPa — Application Logic
   ============================================ */

(function () {
    'use strict';

    // --- Configuration defaults (display limits for progress bars) ---
    const DEFAULT_CFG = {
        maxVoltage: 60,
        maxCurrent: 20,
        maxACVoltage: 250,
        maxACCurrent: 20,
        maxRPM: 3000,
        maxTemp: 100
    };

    let cfg = Object.assign({}, DEFAULT_CFG);

    // --- WebSocket ---
    const gateway = `ws://${window.location.hostname}/ws`;
    let ws = null;
    let wsReconnectTimer = null;

    // Check if running on GitHub Pages (static demo mode)
    const isDemoMode = window.location.hostname.endsWith('.github.io') || 
                       window.location.protocol === 'file:' ||
                       window.location.search.includes('demo=true');

    // Mock store for GitHub Pages config preview
    let demoConfigStore = {
        ssid: "Demo_SSID_AP",
        pass: "12345678",
        pollMs: 100,
        wsPushMs: 500,
        poles: 4,
        rpmMode: 0,
        staEnabled: false,
        staSSID: "Demo_Home_WiFi",
        staPass: "12345678",
        mqttEnabled: false,
        mqttServer: "broker.hivemq.com",
        mqttPort: 1883,
        mqttUser: "",
        mqttPass: "",
        mqttTopic: "sapa/turbine/metrics",
        mqttInterval: 5000,
        maxV: 60,
        maxA: 20,
        maxRPM: 3000,
        maxTemp: 100
    };

    // Client-side API fetch interceptor for static hosting
    function apiFetch(url, options) {
        if (isDemoMode) {
            return new Promise((resolve) => {
                setTimeout(() => {
                    if (url === '/api/config' && options && options.method === 'POST') {
                        const payload = JSON.parse(options.body);
                        Object.assign(demoConfigStore, payload);
                        resolve({
                            json: () => Promise.resolve({ ok: true })
                        });
                    } else if (url === '/api/config') {
                        resolve({
                            json: () => Promise.resolve(demoConfigStore)
                        });
                    } else if (url === '/api/sysinfo') {
                        resolve({
                            json: () => Promise.resolve({
                                fw: "v1.0.0-demo",
                                heap: 245100,
                                uptime: Math.floor(performance.now() / 1000),
                                clients: 1
                            })
                        });
                    } else if (url === '/api/restart') {
                        resolve({
                            json: () => Promise.resolve({ ok: true })
                        });
                    }
                }, 200);
            });
        }
        return fetch(url, options);
    }

    // --- DOM Cache ---
    const $ = (id) => document.getElementById(id);

    const dom = {
        // Dashboard values
        dcPwr:   $('val-dcpwr'),
        dcVolt1: $('val-dcvolt1'),
        dcCur1:  $('val-dccur1'),
        dcVolt2: $('val-dcvolt2'),
        dcCur2:  $('val-dccur2'),
        acVolt1: $('val-acvolt1'),
        acVolt2: $('val-acvolt2'),
        acCur:   $('val-accur'),
        rpm:     $('val-rpm'),
        temp1:   $('val-temp1'),
        temp2:   $('val-temp2'),

        // Progress bars
        barDcVolt1: $('bar-dcvolt1'),
        barDcCur1:  $('bar-dccur1'),
        barDcVolt2: $('bar-dcvolt2'),
        barDcCur2:  $('bar-dccur2'),
        barAcVolt1: $('bar-acvolt1'),
        barAcVolt2: $('bar-acvolt2'),
        barAcCur:   $('bar-accur'),
        barRpm:    $('bar-rpm'),
        barTemp1:   $('bar-temp1'),
        barTemp2:   $('bar-temp2'),

        // Power ring
        powerRing: $('power-ring'),

        // Connection status
        wsDot:       $('ws-dot'),
        wsLabel:     $('ws-label'),
        wsDotMobile: $('ws-dot-mobile'),

        // Uptime
        uptimeLabel: $('uptime-label'),

        // Navigation
        navDashboard: $('nav-dashboard'),
        navSettings:  $('nav-settings'),
        pageDashboard: $('page-dashboard'),
        pageSettings:  $('page-settings'),

        // Mobile
        hamburger: $('hamburger'),
        sidebar:   $('sidebar'),

        // Settings inputs
        cfgSsid:   $('cfg-ssid'),
        cfgPass:   $('cfg-pass'),
        cfgPoll:   $('cfg-poll'),
        cfgWsPush: $('cfg-ws-push'),
        cfgPoles:  $('cfg-poles'),
        cfgRpmMode: $('cfg-rpm-mode'),

        // WiFi STA inputs
        cfgStaEnabled:  $('cfg-sta-enabled'),
        cfgStaSsid:     $('cfg-sta-ssid'),
        cfgStaPass:     $('cfg-sta-pass'),

        // MQTT inputs
        cfgMqttEnabled:  $('cfg-mqtt-enabled'),
        cfgMqttServer:   $('cfg-mqtt-server'),
        cfgMqttPort:     $('cfg-mqtt-port'),
        cfgMqttUser:     $('cfg-mqtt-user'),
        cfgMqttPass:     $('cfg-mqtt-pass'),
        cfgMqttTopic:    $('cfg-mqtt-topic'),
        cfgMqttInterval: $('cfg-mqtt-interval'),

        cfgMaxV:   $('cfg-max-v'),
        cfgMaxA:   $('cfg-max-a'),
        cfgMaxRpm: $('cfg-max-rpm'),
        cfgMaxTemp:$('cfg-max-temp'),

        // Settings buttons
        btnSave:    $('btn-save-cfg'),
        btnRestart: $('btn-restart'),
        togglePass: $('toggle-pass'),
        toggleStaPass: $('toggle-sta-pass'),

        // System info
        sysFw:      $('sys-fw'),
        sysHeap:    $('sys-heap'),
        sysUptime:  $('sys-uptime'),
        sysClients: $('sys-clients'),

        // Toast
        toast:    $('toast'),
        toastMsg: $('toast-msg')
    };

    // --- Navigation ---
    function switchPage(page) {
        document.querySelectorAll('.page').forEach(p => p.classList.remove('active'));
        document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));

        const target = $('page-' + page);
        const btn = $('nav-' + page);
        if (target) target.classList.add('active');
        if (btn) btn.classList.add('active');

        // Close mobile sidebar
        dom.sidebar.classList.remove('open');

        // Load settings when switching to settings page
        if (page === 'settings') {
            loadConfig();
            loadSysInfo();
        }
    }

    dom.navDashboard.addEventListener('click', () => switchPage('dashboard'));
    dom.navSettings.addEventListener('click', () => switchPage('settings'));

    // Mobile hamburger
    dom.hamburger.addEventListener('click', () => {
        dom.sidebar.classList.toggle('open');
    });

    // Close sidebar on backdrop click (mobile)
    document.addEventListener('click', (e) => {
        if (window.innerWidth <= 768 &&
            dom.sidebar.classList.contains('open') &&
            !dom.sidebar.contains(e.target) &&
            !dom.hamburger.contains(e.target)) {
            dom.sidebar.classList.remove('open');
        }
    });

    // --- Password toggle ---
    dom.togglePass.addEventListener('click', () => {
        const inp = dom.cfgPass;
        inp.type = inp.type === 'password' ? 'text' : 'password';
    });
    dom.toggleStaPass.addEventListener('click', () => {
        const inp = dom.cfgStaPass;
        inp.type = inp.type === 'password' ? 'text' : 'password';
    });

    // --- Toast notifications ---
    let toastTimer = null;
    function showToast(msg, type) {
        dom.toastMsg.textContent = msg;
        dom.toast.className = 'toast show ' + (type || '');
        clearTimeout(toastTimer);
        toastTimer = setTimeout(() => {
            dom.toast.className = 'toast';
        }, 3000);
    }

    // --- WebSocket Connection ---
    let demoInterval = null;
    function startDemoSimulation() {
        if (demoInterval) return;
        setConnectionStatus(true);
        if (dom.wsLabel) dom.wsLabel.textContent = 'Live (Demo)';

        demoInterval = setInterval(() => {
            const now = Date.now();
            
            const dcV1 = 24.0 + 2.5 * Math.sin(now / 5000);
            const dcA1 = 5.0 + 3.0 * Math.sin(now / 3000);
            const dcP1 = dcV1 * dcA1;

            const dcV2 = 12.0 + 1.2 * Math.sin(now / 6000);
            const dcA2 = 2.0 + 1.0 * Math.sin(now / 4000);
            const dcP2 = dcV2 * dcA2;

            const acV1 = 220.0 + 10.0 * Math.sin(now / 4000);
            const acV2 = 218.0 + 8.0 * Math.sin(now / 4500);
            const acA  = 1.5 + 0.5 * Math.sin(now / 3500);

            const rpm = 1200.0 + 400.0 * Math.sin(now / 6000);
            const t1 = 42.5 + 3.5 * Math.sin(now / 8000);
            const t2 = 28.5 + 1.5 * Math.sin(now / 9000);

            const mockData = {
                dcV1: dcV1,
                dcA1: dcA1 > 0 ? dcA1 : 0,
                dcP1: dcP1 > 0 ? dcP1 : 0,
                dcV2: dcV2,
                dcA2: dcA2 > 0 ? dcA2 : 0,
                dcP2: dcP2 > 0 ? dcP2 : 0,
                acV1: acV1,
                acV2: acV2,
                acA: acA > 0 ? acA : 0,
                rpm: rpm > 0 ? rpm : 0,
                t1: t1,
                t2: t2,
                uptime: Math.floor(performance.now() / 1000)
            };
            updateDashboard(mockData);
        }, demoConfigStore.wsPushMs || 500);
    }

    function connectWS() {
        if (isDemoMode) {
            startDemoSimulation();
            return;
        }

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

    // --- Dashboard Update ---
    function updateDashboard(data) {
        // Update text values
        setText(dom.dcPwr, data.dcP1 != null ? data.dcP1.toFixed(2) : '0.00');
        
        setText(dom.dcVolt1, data.dcV1 != null ? data.dcV1.toFixed(2) : '0.00');
        setText(dom.dcCur1, data.dcA1 != null ? data.dcA1.toFixed(2) : '0.00');
        setText(dom.dcVolt2, data.dcV2 != null ? data.dcV2.toFixed(2) : '0.00');
        setText(dom.dcCur2, data.dcA2 != null ? data.dcA2.toFixed(2) : '0.00');
        
        setText(dom.acVolt1, data.acV1 != null ? data.acV1.toFixed(1) : '0.0');
        setText(dom.acVolt2, data.acV2 != null ? data.acV2.toFixed(1) : '0.0');
        setText(dom.acCur, data.acA != null ? data.acA.toFixed(2) : '0.00');
        
        setText(dom.rpm, data.rpm != null ? Math.round(data.rpm).toString() : '0');
        setText(dom.temp1, data.t1 != null ? data.t1.toFixed(1) : '0.0');
        setText(dom.temp2, data.t2 != null ? data.t2.toFixed(1) : '0.0');

        // Update progress bars (clamped 0-100%)
        setBar(dom.barDcVolt1, data.dcV1, cfg.maxVoltage);
        setBar(dom.barDcCur1, data.dcA1, cfg.maxCurrent);
        setBar(dom.barDcVolt2, data.dcV2, cfg.maxVoltage);
        setBar(dom.barDcCur2, data.dcA2, cfg.maxCurrent);
        
        setBar(dom.barAcVolt1, data.acV1, cfg.maxACVoltage);
        setBar(dom.barAcVolt2, data.acV2, cfg.maxACVoltage);
        setBar(dom.barAcCur, data.acA, cfg.maxACCurrent);
        
        setBar(dom.barRpm, data.rpm, cfg.maxRPM);
        setBar(dom.barTemp1, data.t1, cfg.maxTemp);
        setBar(dom.barTemp2, data.t2, cfg.maxTemp);

        // Update power ring
        updatePowerRing(data.dcP1 || 0);

        // Update uptime if present
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
        // circumference = 2 * PI * 52 ≈ 326.7
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

    // --- Settings: Load Config from ESP32 ---
    function loadConfig() {
        apiFetch('/api/config')
            .then(r => r.json())
            .then(data => {
                if (data.ssid != null)    dom.cfgSsid.value = data.ssid;
                if (data.pass != null)    dom.cfgPass.value = data.pass;
                if (data.pollMs != null)  dom.cfgPoll.value = data.pollMs;
                if (data.wsPushMs != null) dom.cfgWsPush.value = data.wsPushMs;
                if (data.poles != null)   dom.cfgPoles.value = data.poles;
                if (data.rpmMode != null) dom.cfgRpmMode.value = data.rpmMode;

                // WiFi STA configuration load
                if (data.staEnabled != null)  dom.cfgStaEnabled.checked = data.staEnabled;
                if (data.staSSID != null)     dom.cfgStaSsid.value = data.staSSID;
                if (data.staPass != null)     dom.cfgStaPass.value = data.staPass;

                // MQTT configuration load
                if (data.mqttEnabled != null)  dom.cfgMqttEnabled.checked = data.mqttEnabled;
                if (data.mqttServer != null)   dom.cfgMqttServer.value = data.mqttServer;
                if (data.mqttPort != null)     dom.cfgMqttPort.value = data.mqttPort;
                if (data.mqttUser != null)     dom.cfgMqttUser.value = data.mqttUser;
                if (data.mqttPass != null)     dom.cfgMqttPass.value = data.mqttPass;
                if (data.mqttTopic != null)    dom.cfgMqttTopic.value = data.mqttTopic;
                if (data.mqttInterval != null) dom.cfgMqttInterval.value = data.mqttInterval;

                if (data.maxV != null)    { dom.cfgMaxV.value = data.maxV; cfg.maxVoltage = data.maxV; }
                if (data.maxA != null)    { dom.cfgMaxA.value = data.maxA; cfg.maxCurrent = data.maxA; }
                if (data.maxRPM != null)  { dom.cfgMaxRpm.value = data.maxRPM; cfg.maxRPM = data.maxRPM; }
                if (data.maxTemp != null) { dom.cfgMaxTemp.value = data.maxTemp; cfg.maxTemp = data.maxTemp; }
            })
            .catch(() => {
                // Silently fail — defaults are fine on first load
            });
    }

    // --- Settings: Load System Info ---
    function loadSysInfo() {
        apiFetch('/api/sysinfo')
            .then(r => r.json())
            .then(data => {
                if (data.fw != null)      dom.sysFw.textContent = data.fw;
                if (data.heap != null)    dom.sysHeap.textContent = (data.heap / 1024).toFixed(1) + ' KB';
                if (data.uptime != null)  dom.sysUptime.textContent = formatUptime(data.uptime);
                if (data.clients != null) dom.sysClients.textContent = data.clients;
            })
            .catch(() => {});
    }

    // --- Settings: Save Config ---
    dom.btnSave.addEventListener('click', () => {
        const payload = {};

        if (dom.cfgSsid.value)    payload.ssid = dom.cfgSsid.value;
        if (dom.cfgPass.value)    payload.pass = dom.cfgPass.value;
        if (dom.cfgPoll.value)    payload.pollMs = parseInt(dom.cfgPoll.value, 10);
        if (dom.cfgWsPush.value)  payload.wsPushMs = parseInt(dom.cfgWsPush.value, 10);
        if (dom.cfgPoles.value)   payload.poles = parseInt(dom.cfgPoles.value, 10);
        if (dom.cfgRpmMode.value !== '') payload.rpmMode = parseInt(dom.cfgRpmMode.value, 10);

        // WiFi STA configuration save
        payload.staEnabled = dom.cfgStaEnabled.checked;
        payload.staSSID = dom.cfgStaSsid.value;
        payload.staPass = dom.cfgStaPass.value;

        // MQTT configuration save
        payload.mqttEnabled = dom.cfgMqttEnabled.checked;
        payload.mqttServer = dom.cfgMqttServer.value;
        payload.mqttPort = parseInt(dom.cfgMqttPort.value, 10) || 1883;
        payload.mqttUser = dom.cfgMqttUser.value;
        payload.mqttPass = dom.cfgMqttPass.value;
        payload.mqttTopic = dom.cfgMqttTopic.value;
        payload.mqttInterval = parseInt(dom.cfgMqttInterval.value, 10) || 5000;

        if (dom.cfgMaxV.value)    payload.maxV = parseFloat(dom.cfgMaxV.value);
        if (dom.cfgMaxA.value)    payload.maxA = parseFloat(dom.cfgMaxA.value);
        if (dom.cfgMaxRpm.value)  payload.maxRPM = parseInt(dom.cfgMaxRpm.value, 10);
        if (dom.cfgMaxTemp.value) payload.maxTemp = parseInt(dom.cfgMaxTemp.value, 10);

        apiFetch('/api/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(payload)
        })
        .then(r => r.json())
        .then(data => {
            if (data.ok) {
                showToast('Configuration saved successfully!', 'success');
                // Update local display limits
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

    // --- Settings: Restart ---
    dom.btnRestart.addEventListener('click', () => {
        if (!confirm('Restart the device? You will be disconnected briefly.')) return;

        apiFetch('/api/restart', { method: 'POST' })
            .then(() => {
                showToast('Device restarting...', 'success');
                setTimeout(() => { window.location.reload(); }, 5000);
            })
            .catch(() => {
                showToast('Restart command sent', 'success');
                setTimeout(() => { window.location.reload(); }, 5000);
            });
    });

    // --- Initialize ---
    connectWS();
    loadConfig();

})();
