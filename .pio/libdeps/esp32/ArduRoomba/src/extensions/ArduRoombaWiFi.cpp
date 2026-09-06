/**
 * @file ArduRoombaWiFi.cpp
 * @brief Implementation of base WiFi functionality
 */

#include "ArduRoombaWiFi.h"

ArduRoombaWiFi::ArduRoombaWiFi(ArduRoomba& roomba)
  : _roomba(roomba), _remoteEnabled(true), _commandCallback(nullptr),
    _lowBatteryThreshold(12000), _serverPort(80) {
}

CommandResult ArduRoombaWiFi::processCommand(const RoombaCommand& cmd) {
  if (!_remoteEnabled) {
    return CommandResult::ERROR;
  }

  // Validate action
  String action = String(cmd.action);
  if (!isValidAction(action)) {
    return CommandResult::UNKNOWN_ACTION;
  }

  // Check battery
  uint16_t voltage = _roomba.getBatteryVoltage();
  if (voltage > 0 && voltage < _lowBatteryThreshold) {
    return CommandResult::LOW_BATTERY;
  }

  // Call user callback if set
  if (_commandCallback) {
    _commandCallback(cmd, CommandResult::SUCCESS);
  }

  // Process standard commands
  if (action == "forward") {
    _roomba.moveForward(cmd.speed > 0 ? cmd.speed : 200);
  }
  else if (action == "backward") {
    _roomba.moveBackward(cmd.speed > 0 ? cmd.speed : 200);
  }
  else if (action == "left") {
    _roomba.turnLeft(cmd.speed > 0 ? cmd.speed : 200);
  }
  else if (action == "right") {
    _roomba.turnRight(cmd.speed > 0 ? cmd.speed : 200);
  }
  else if (action == "spinLeft") {
    _roomba.spinLeft(cmd.speed > 0 ? cmd.speed : 200);
  }
  else if (action == "spinRight") {
    _roomba.spinRight(cmd.speed > 0 ? cmd.speed : 200);
  }
  else if (action == "stop") {
    _roomba.stop();
  }
  else if (action == "clean") {
    _roomba.startCleaning();
  }
  else if (action == "spot") {
    _roomba.spotClean();
  }
  else if (action == "dock") {
    _roomba.dock();
  }
  else if (action == "beep") {
    _roomba.beep();
  }

  // Handle timed commands
  if (cmd.duration > 0 && action != "stop" && action != "clean" &&
      action != "spot" && action != "dock") {
    delay(cmd.duration);
    _roomba.stop();
  }

  return CommandResult::SUCCESS;
}

bool ArduRoombaWiFi::isValidAction(const String& action) const {
  return action == "forward" || action == "backward" ||
         action == "left" || action == "right" ||
         action == "spinLeft" || action == "spinRight" ||
         action == "stop" || action == "clean" ||
         action == "spot" || action == "dock" || action == "beep";
}

String ArduRoombaWiFi::generateControlPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ArduRoomba Control & Map</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, sans-serif;
      text-align: center;
      background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
      color: #fff;
      padding: 10px;
    }
    .container { max-width: 500px; margin: 0 auto; }
    
    /* Stili per il Canvas della Mappa */
    .map-container {
      background: #2c3e50; /* Colore UNKNOWN di default */
      border-radius: 8px;
      padding: 5px;
      margin-bottom: 15px;
      box-shadow: 0 4px 6px rgba(0,0,0,0.3);
      overflow: hidden;
    }
    canvas {
      width: 100%;
      height: auto;
      background-color: #34495e; /* Grigio scuro per nebbia di guerra */
      display: block;
      image-rendering: pixelated; /* Mantiene nitidi i pixel della mappa */
    }
    .legend {
      display: flex;
      justify-content: center;
      gap: 10px;
      font-size: 12px;
      margin-bottom: 15px;
      flex-wrap: wrap;
    }
    .legend span { display: flex; align-items: center; gap: 4px; }
    .color-box { width: 12px; height: 12px; border-radius: 2px; }

    /* Controlli UI (esistenti ma ottimizzati) */
    .status { background: rgba(255,255,255,0.1); border-radius: 8px; padding: 10px; margin-bottom: 15px; }
    .status-item { display: flex; justify-content: space-between; margin: 5px 0; }
    .controls { display: grid; grid-template-columns: repeat(3, 1fr); gap: 5px; margin-bottom: 15px; }
    button { padding: 15px; font-size: 20px; background: rgba(255,255,255,0.2); border: none; border-radius: 8px; color: white; cursor: pointer; }
    button:hover { background: rgba(255,255,255,0.3); }
    .stop { background: rgba(231, 76, 60, 0.8) !important; font-size: 16px; font-weight: bold; }
    .actions { display: flex; gap: 5px; justify-content: center; }
    .actions button { padding: 10px; font-size: 14px; background: rgba(39, 174, 96, 0.8); flex: 1; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🤖 ArduRoomba</h1>
    
    <!-- CANVAS DELLA MAPPA -->
    <div class="map-container">
      <canvas id="mapCanvas" width="300" height="300"></canvas>
    </div>
    
    <!-- LEGENDA MAPPA -->
    <div class="legend">
      <span><div class="color-box" style="background:#ecf0f1"></div> Libero</span>
      <span><div class="color-box" style="background:#e74c3c"></div> Ostacolo</span>
      <span><div class="color-box" style="background:#8e44ad"></div> Sporco</span>
      <span><div class="color-box" style="background:#3498db"></div> Passato</span>
      <span>🤖 Robot</span>
    </div>

    <div class="status">
      <div class="status-item"><span>Battery:</span><span id="voltage">-- mV</span></div>
      <div class="status-item"><span>Status:</span><span id="status">Connecting...</span></div>
    </div>
    
    <div class="controls">
      <button onclick="send('spinLeft')">↺</button>
      <button onclick="send('forward')">▲</button>
      <button onclick="send('spinRight')">↻</button>
      <button onclick="send('left')">◀</button>
      <button class="stop" onclick="send('stop')">⏹</button>
      <button onclick="send('right')">▶</button>
      <div></div>
      <button onclick="send('backward')">▼</button>
      <div></div>
    </div>
    <div class="actions">
      <button onclick="send('clean')">🧹 Clean</button>
      <button onclick="send('spot')">⚡ Spot</button>
      <button onclick="send('dock')">🏠 Dock</button>
    </div>
  </div>

  <script>
    // --- MAP DRAWING LOGIC ---
    const canvas = document.getElementById('mapCanvas');
    const ctx = canvas.getContext('2d');
    
    // Mappatura colori in base all'enum cellStatus di C++
    const colors = {
      1: '#ecf0f1', // FREE (Bianco)
      2: '#e74c3c', // OBSTACLE (Rosso)
      3: '#8e44ad', // DIRT (Viola)
      4: '#3498db'  // PASSED (Azzurro)
    };

    function drawMap(data) {
      if (!data || !data.w) return;
      
      // Assicurati che il canvas abbia le proporzioni giuste
      if (canvas.width !== data.w) canvas.width = data.w;
      if (canvas.height !== data.h) canvas.height = data.h;
      
      const cellW = 1; // 1 pixel per cella (viene poi scalato dal CSS)
      const cellH = 1;

      // Svuota tutto mettendo il colore di default (UNKNOWN)
      ctx.fillStyle = '#34495e';
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      // Disegna solo le celle conosciute
      data.cells.forEach(cell => {
        const x = cell[0];
        const y = cell[1];
        const state = cell[2];
        ctx.fillStyle = colors[state] || '#34495e';
        ctx.fillRect(x, y, cellW, cellH);
      });

      // Disegna il Robot (un pallino giallo/verde per spiccare)
      ctx.fillStyle = '#f1c40f';
      ctx.beginPath();
      // Disegna un cerchio leggermente più grande di un pixel per vederlo
      ctx.arc(data.cx, data.cy, 2, 0, 2 * Math.PI); 
      ctx.fill();
    }

    function fetchMap() {
      fetch('/map')
        .then(r => r.json())
        .then(data => drawMap(data))
        .catch(e => console.error("Map Error:", e));
    }

    // --- CONTROLS LOGIC ---
    function send(action) {
      fetch('/cmd?action=' + action + '&speed=200').catch(e => console.error(e));
    }

    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(d => {
          document.getElementById('voltage').textContent = d.voltage + ' mV';
          const statusEl = document.getElementById('status');
          if (d.voltage < 13000) { statusEl.textContent = '⚠️ Low Battery'; statusEl.style.color = '#f1c40f'; } 
          else if (d.bumper) { statusEl.textContent = '⚠️ Bumper Hit'; statusEl.style.color = '#e74c3c'; } 
          else { statusEl.textContent = '✓ Ready'; statusEl.style.color = '#2ecc71'; }
        }).catch(e => console.error(e));
    }

    // Aggiornamenti periodici (Mappa ogni 1.5s, Status ogni 2s)
    setInterval(fetchMap, 1500);
    setInterval(updateStatus, 2000);
    
    // Prima esecuzione
    fetchMap();
    updateStatus();
  </script>
</body>
</html>
)rawliteral";
  return html;
}

String ArduRoombaWiFi::generateStatusJSON() {
  uint16_t voltage = _roomba.getBatteryVoltage();
  bool connected = _roomba.isConnected();
  bool bumper = _roomba.isBumperPressed();
  bool wall = _roomba.isWallDetected(false);

  String json = "{";
  json += "\"voltage\":" + String(voltage) + ",";
  json += "\"connected\":" + String(connected ? "true" : "false") + ",";
  json += "\"bumper\":" + String(bumper ? "true" : "false") + ",";
  json += "\"wall\":" + String(wall ? "true" : "false") + ",";
  json += "\"remote_enabled\":" + String(_remoteEnabled ? "true" : "false");
  json += "}";

  return json;
}

String ArduRoombaWiFi::generateExtendedStatusJSON() {
  uint16_t voltage = _roomba.getBatteryVoltage();
  int16_t current = _roomba.getBatteryCurrent();
  uint8_t percent = _roomba.getBatteryPercent();
  bool connected = _roomba.isConnected();
  bool bumper = _roomba.isBumperPressed();
  bool wall = _roomba.isWallDetected(false);
  bool cliff = _roomba.isCliffDetected();

  String json = "{";
  json += "\"voltage\":" + String(voltage) + ",";
  json += "\"current\":" + String(current) + ",";
  json += "\"battery_percent\":" + String(percent) + ",";
  json += "\"connected\":" + String(connected ? "true" : "false") + ",";
  json += "\"bumper\":" + String(bumper ? "true" : "false") + ",";
  json += "\"wall\":" + String(wall ? "true" : "false") + ",";
  json += "\"cliff\":" + String(cliff ? "true" : "false") + ",";
  json += "\"remote_enabled\":" + String(_remoteEnabled ? "true" : "false") + ",";
  json += "\"mode\":\"" + getModeString() + "\",";
  json += "\"ip\":\"" + getIPAddress() + "\"";
  json += "}";

  return json;
}

void ArduRoombaWiFi::startWebServer(uint16_t port) {
  _serverPort = port;
  // Implemented by platform-specific class
}
