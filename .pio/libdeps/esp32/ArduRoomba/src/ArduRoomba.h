/**
 * @file ArduRoomba.h
 * @brief Main ArduRoomba library interface (Refactored)
 *
 * Clean, modular interface for controlling iRobot Create 2 and compatible Roomba models.
 * Provides simple high-level methods while maintaining modularity for extensions.
 *
 * Architecture:
 * - RoombaSerial: Abstract serial communication (SoftwareSerial/HardwareSerial)
 * - RoombaSensors: Structured sensor data access
 * - RoombaMovement: Fluent movement control with sequences
 * - RoombaActuators: LED, motor, and sound control
 * - RoombaSafety: Battery monitoring and obstacle detection
 */

#ifndef ARDUROOMBA_H
#define ARDUROOMBA_H

#include <Arduino.h>

// Include core components
#include "core/RoombaSerial.h"
#include "core/RoombaSensors.h"
#include "core/RoombaMovement.h"
#include "core/RoombaActuators.h"

/**
 * Roomba configuration structure
 * Centralizes all configurable parameters
 */
struct RoombaConfig {
  uint8_t rxPin;           // RX pin for serial communication
  uint8_t txPin;           // TX pin for serial communication
  uint8_t brcPin;          // BRC (Baud Rate Change) / Device Detect pin
  uint32_t baudRate;       // Serial baud rate (default: 19200)
  bool useHardwareSerial;  // Use HardwareSerial if available
  HardwareSerial* hwSerial; // HardwareSerial instance (if useHardwareSerial)
  uint16_t lowBatteryThreshold;  // Low battery threshold in mV
  uint16_t criticalBatteryThreshold; // Critical battery threshold in mV
  bool enableSafety;       // Enable automatic safety features

  // Default configuration for software serial
  static RoombaConfig createDefault(const uint8_t rx = 2, const uint8_t tx = 3, const uint8_t brc = 4) {
    RoombaConfig config{};
    config.rxPin = rx;
    config.txPin = tx;
    config.brcPin = brc;
    config.baudRate = 19200;
    config.useHardwareSerial = false;
    config.hwSerial = nullptr;
    config.lowBatteryThreshold = 13000;
    config.criticalBatteryThreshold = 12000;
    config.enableSafety = true;
    return config;
  }

  #if defined(ESP32)
  // Configuration for ESP32 with hardware serial (Serial2 by default)
  static RoombaConfig createESP32(HardwareSerial* serial = &Serial2, const uint8_t brc = 5) {
    RoombaConfig config{};
    config.rxPin = 16;  // Default ESP32 RX2
    config.txPin = 17;  // Default ESP32 TX2
    config.brcPin = brc;
    config.baudRate = 19200;
    config.useHardwareSerial = true;
    config.hwSerial = serial;
    config.lowBatteryThreshold = 13000;
    config.criticalBatteryThreshold = 12000;
    config.enableSafety = true;
    return config;
  }
  #elif defined(ESP8266)
  // Configuration for ESP8266 with hardware serial
  static RoombaConfig createESP8266(uint8_t brc = 5) {
    RoombaConfig config;
    config.rxPin = 3;   // ESP8266 RX
    config.txPin = 1;   // ESP8266 TX
    config.brcPin = brc;
    config.baudRate = 19200;
    config.useHardwareSerial = true;
    config.hwSerial = &Serial;
    config.lowBatteryThreshold = 13000;
    config.criticalBatteryThreshold = 12000;
    config.enableSafety = true;
    return config;
  }
  #endif

  #if defined(ARDUINO_UNOWIFIR4) || defined(ARDUINO_UNOR4_WIFI)
  // Configuration for Uno R4 WiFi with hardware serial (Serial1)
  static RoombaConfig createUnoR4(uint8_t brc = 4) {
    RoombaConfig config;
    config.rxPin = 0;   // Serial1 RX on Uno R4
    config.txPin = 1;   // Serial1 TX on Uno R4
    config.brcPin = brc;
    config.baudRate = 19200;
    config.useHardwareSerial = true;
    config.hwSerial = &Serial1;
    config.lowBatteryThreshold = 13000;
    config.criticalBatteryThreshold = 12000;
    config.enableSafety = true;
    return config;
  }
  #endif
};

/**
 * Main ArduRoomba class
 * Provides unified access to all Roomba functionality
 */
class ArduRoomba {
public:
  // Constructor with pin configuration
  ArduRoomba(uint8_t rxPin, uint8_t txPin, uint8_t brcPin);

  explicit ArduRoomba(const RoombaConfig& config);

  // Destructor
  ~ArduRoomba();
  // Initialization
  bool begin(uint32_t baudRate);
  void end();
  bool isConnected() const { return _connected; }

  // Component access
  RoombaSensors& sensors() const { return *_sensors; }
  RoombaMovement& movement() const { return *_movement; }
  RoombaActuators& actuators() const { return *_actuators; }
  RoombaSerial* serial() const { return _serial; }

  // Quick access to common movement commands
  void stop() const { _movement->stop(); }
  void moveForward(const int16_t speed = 200) const { _movement->moveForward(speed); }
  void moveBackward(const int16_t speed = 200) const { _movement->moveBackward(speed); }
  void turnLeft(const int16_t speed = 200) const { _movement->turnLeft(speed); }
  void turnRight(const int16_t speed = 200) const { _movement->turnRight(speed); }
  void spinLeft(const int16_t speed = 200) const { _movement->spinLeft(speed); }
  void spinRight(const int16_t speed = 200) const { _movement->spinRight(speed); }

  // Quick access to sensor readings
  short getDistance() const { return _sensors->getDistance(); }
  short getAngle() const { return _sensors->getAngle(); }
  uint16_t getBatteryVoltage() const { return _sensors->getBatteryVoltage(); }
  int16_t getBatteryCurrent() const { return _sensors->getBatteryCurrent(); }
  uint8_t getBatteryPercent() const { return _sensors->getBatteryPercent(); }
  bool isBatteryLow() const { return _sensors->isBatteryLow(); }
  bool isBatteryCritical() const{ return _sensors->isBatteryCritical(); }
  bool isBumperPressed() const { return _sensors->isBumperPressed(); }
  BumperData readBumpers() const { return _sensors->readBumpers(); }
  bool isWallDetected(const bool quick) const { return _sensors->isWallDetected(quick); }
  bool isCliffDetected() const { return _sensors->isCliffDetected(); }
  uint8_t getDirt() const { return _sensors->getDirt(); }
  bool isObstacle() const { return _sensors->isObstacle(); }

  // Quick access to actuator control
  void setLED(const bool debris, const bool spot, const bool dock, const bool checkRobot) const {
    _actuators->setLED(debris, spot, dock, checkRobot);
  }
  void setPowerLED(const uint8_t color, const uint8_t intensity = 255) const {
    _actuators->setPowerLED(color, intensity);
  }
  void setMotors(const bool mainBrush, const bool sideBrush, const bool vacuum) const {
    _actuators->setMotors(mainBrush, sideBrush, vacuum);
  }
  void beep() const { _actuators->beep(); }

  // Cleaning modes
  void startCleaning() const { _actuators->startCleaning(); }
  void spotClean() const { _actuators->startSpotClean(); }
  void dock() const { _actuators->seekDock(); }

  // Mode control
  void setSafeMode() const { _actuators->setSafeMode(); }
  void setFullMode() const { _actuators->setFullMode(); }

  // Configuration
  const RoombaConfig& getConfig() const { return _config; }
  void setConfig(const RoombaConfig& config) { _config = config; }

  // Debug
  void setDebug(bool enable);
  bool isDebug() const { return _debug; }

  // Safety features
  void enableSafety(const bool enable) { _config.enableSafety = enable; }
  bool isSafetyEnabled() const { return _config.enableSafety; }
  void updateSafety(); // Call in loop() for automatic safety features

  // Legacy support - getOI() for backward compatibility
  class RoombaOI_Legacy;
  RoombaOI_Legacy& getOI();

private:
  RoombaConfig _config;
  RoombaSerial* _serial;
  RoombaSensors* _sensors;
  RoombaMovement* _movement;
  RoombaActuators* _actuators;
  RoombaOI_Legacy* _legacyOI;
  bool _connected;
  bool _debug;

  // Internal methods
  bool initSerial();
  void pulseBRC();
  void sendStartCommand();
};

/**
 * Legacy compatibility wrapper
 * Provides old RoombaOI interface using new components
 */
class ArduRoomba::RoombaOI_Legacy {
public:
  explicit RoombaOI_Legacy(ArduRoomba* parent) : _parent(parent) {}

  // Legacy methods mapped to new architecture
  void drive(const int16_t velocity, const int16_t radius) const {
    _parent->_movement->drive(velocity, radius);
  }

  void driveDirect(const int16_t rightVel, const int16_t leftVel) const {
    _parent->_movement->driveDirect(rightVel, leftVel);
  }

  void stop() const { _parent->_movement->stop(); }

  void setMotors(const bool mainBrush, const bool sideBrush, const bool vacuum) const {
    _parent->_actuators->setMotors(mainBrush, sideBrush, vacuum);
  }

  void setLEDs(const uint8_t ledBits, const uint8_t powerColor, const uint8_t powerIntensity) const {
    _parent->_actuators->setAllLEDs(ledBits, powerColor, powerIntensity);
  }

  uint16_t getBatteryVoltage() const { return _parent->_sensors->getBatteryVoltage(); }
  int16_t getBatteryCurrent() const { return _parent->_sensors->getBatteryCurrent(); }
  bool isWallDetected(const bool quick) const { return _parent->_sensors->isWallDetected(quick); }
  bool isBumperPressed() const { return _parent->_sensors->isBumperPressed(); }

  void setDebug(const bool enable) const {
    _parent->_sensors->setDebug(enable);
    _parent->_movement->setDebug(enable);
    _parent->_actuators->setDebug(enable);
  }

  void sendCommand(const uint8_t cmd) const {
    if (_parent->_serial && _parent->_serial->isActive()) {
      _parent->_serial->write(cmd);
    }
  }

  void sendCommand(const uint8_t cmd, const uint8_t* params, const uint8_t numParams) const {
    if (_parent->_serial && _parent->_serial->isActive() && params) {
      _parent->_serial->write(cmd);
      _parent->_serial->write(params, numParams);
    }
  }

private:
  ArduRoomba* _parent;
};

// Global sequence builder helper
inline RoombaSequence RoombaSequenceBuilder(const ArduRoomba& roomba) {
  return RoombaSequence(&roomba.movement());
}

#endif
