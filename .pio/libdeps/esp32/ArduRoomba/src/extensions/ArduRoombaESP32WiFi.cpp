/**
 * @file ArduRoombaESP32WiFi.cpp
 * @brief Implementation of WiFi control for ESP32
 */

#include "ArduRoombaESP32WiFi.h"

#if defined(ESP32)

ArduRoombaESP32WiFi::ArduRoombaESP32WiFi(ArduRoomba& roomba)
  : ArduRoombaWiFi(roomba), _server(nullptr), _mode(static_cast<WiFiMode>(WIFI_MODE_NULL)), _connected(false) {
}

ArduRoombaESP32WiFi::~ArduRoombaESP32WiFi() {
  end();
}

bool ArduRoombaESP32WiFi::beginAP(const char* ssid, const char* password) {
  Serial.print("Creating WiFi AP: ");
  Serial.println(ssid);

  _mode = static_cast<WiFiMode>(WIFI_AP);

  // Create access point
  bool success;
  if (password && strlen(password) > 0) {
    success = WiFi.softAP(ssid, password);
  } else {
    success = WiFi.softAP(ssid);
  }

  if (!success) {
    Serial.println("Failed to create AP");
    return false;
  }

  delay(100);

  const IPAddress ip = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(ip);

  _connected = true;
  return true;
}

bool ArduRoombaESP32WiFi::beginClient(const char* ssid, const char* password) {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  _mode = static_cast<WiFiMode>(WIFI_STA);

  // Connect to WiFi
  WiFi.begin(ssid, password);

  // Wait for connection
  int attempts = 0;
  while (WiFiClass::status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();

  if (WiFiClass::status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi");
    return false;
  }

  IPAddress ip = WiFi.localIP();
  Serial.print("Connected! IP address: ");
  Serial.println(ip);

  _connected = true;
  return true;
}

void ArduRoombaESP32WiFi::end() {
  if (_server) {
    _server->stop();
    delete _server;
    _server = nullptr;
  }

  if (_mode == static_cast<WiFiMode>(WIFI_AP)) {
    WiFi.softAPdisconnect(true);
  } else if (_mode == static_cast<WiFiMode>(WIFI_STA)) {
    WiFi.disconnect(true);
  }

  _connected = false;
}

bool ArduRoombaESP32WiFi::isConnected() const {
  if (_mode == static_cast<WiFiMode>(WIFI_STA)) {
    return WiFiClass::status() == WL_CONNECTED;
  }
  return _connected;
}

String ArduRoombaESP32WiFi::getModeString() const {
  if (_mode == static_cast<WiFiMode>(WIFI_AP)) return "AP";
  if (_mode == static_cast<WiFiMode>(WIFI_STA)) return "Client";
  return "Unknown";
}

String ArduRoombaESP32WiFi::getIPAddress() const {
  if (_mode == static_cast<WiFiMode>(WIFI_AP)) {
    return WiFi.softAPIP().toString();
  }
  return WiFi.localIP().toString();
}

String ArduRoombaESP32WiFi::getMACAddress() const {
  return WiFi.macAddress();
}

int ArduRoombaESP32WiFi::getRSSI() const {
  if (_mode == static_cast<WiFiMode>(WIFI_STA) && WiFiClass::status() == WL_CONNECTED) {
    return WiFi.RSSI();
  }
  return 0;
}

void ArduRoombaESP32WiFi::startWebServer(const uint16_t port) {
  if (_server) {
    _server->stop();
    delete _server;
  }

  _server = new WebServer(port);
  _serverPort = port;

  // Setup routes
  _server->on("/", [this]() { handleRoot(); });
  _server->on("/cmd", HTTP_GET, [this]() { handleCommand(); });
  _server->on("/status", [this]() { handleStatus(); });
  _server->onNotFound([this]() { handleNotFound(); });

  _server->begin();

  Serial.print("Web server started on port ");
  Serial.println(port);
  Serial.print("Access at: http://");
  Serial.println(getIPAddress());
}

void ArduRoombaESP32WiFi::handleClient() {
  if (_server) {
    _server->handleClient();
  }
}

void ArduRoombaESP32WiFi::handleRoot() {
  const String html = generateControlPage();
  _server->send(200, "text/html; charset=utf-8", html);
}

void ArduRoombaESP32WiFi::handleCommand() {
  const RoombaCommand cmd = parseCommand();

  const CommandResult result = processCommand(cmd);

  if (result == CommandResult::SUCCESS) {
    _server->sendHeader("Access-Control-Allow-Origin", "*");
    _server->send(200, "text/plain", "OK");
  } else if (result == CommandResult::LOW_BATTERY) {
    _server->sendHeader("Access-Control-Allow-Origin", "*");
    _server->send(503, "text/plain", "Low Battery");
  } else {
    _server->sendHeader("Access-Control-Allow-Origin", "*");
    _server->send(400, "text/plain", "Bad Request");
  }
}

void ArduRoombaESP32WiFi::handleStatus() const {
  const String json = generateStatusJSON();
  _server->sendHeader("Access-Control-Allow-Origin", "*");
  _server->send(200, "application/json", json);
}

void ArduRoombaESP32WiFi::handleNotFound() const {
  _server->send(404, "text/plain", "Not Found");
}

RoombaCommand ArduRoombaESP32WiFi::parseCommand() const {
  RoombaCommand cmd;

  if (_server->hasArg("action")) {
    const String action = _server->arg("action");
    strncpy(cmd.action, action.c_str(), sizeof(cmd.action) - 1);
    cmd.action[sizeof(cmd.action) - 1] = '\0';
  }

  if (_server->hasArg("speed")) {
    cmd.speed = _server->arg("speed").toInt();
  }

  if (_server->hasArg("duration")) {
    cmd.duration = _server->arg("duration").toInt();
  }

  return cmd;
}

#endif // ESP32
