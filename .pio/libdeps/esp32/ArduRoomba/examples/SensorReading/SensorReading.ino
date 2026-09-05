/**
 * SensorReading.ino
 * 
 * Example showing how to read basic sensor data from the Roomba.
 * Displays battery status, wall detection, and bumper status.
 */

#include "ArduRoomba.h"

ArduRoomba roomba(2, 3, 4);

void setup() {
  Serial.begin(19200);
  
  roomba.setDebug(true);
  
  if (roomba.begin()) {
    Serial.println("Roomba connected!");
    Serial.println("Starting sensor monitoring...");
    Serial.println("Press bumpers or move near walls to see sensor changes.");
    Serial.println();
  } else {
    Serial.println("Failed to connect to Roomba!");
    while(1); // Stop here
  }
}

void loop() {
  // Read and display sensor data every 2 seconds
  readSensors();
  delay(2000);
}

void readSensors() {
  Serial.println("=== Sensor Reading ===");
  
  // Battery information
  uint16_t voltage = roomba.getBatteryVoltage();
  int16_t current = roomba.getBatteryCurrent();
  
  Serial.print("Battery Voltage: ");
  Serial.print(voltage);
  Serial.println(" mV");
  
  Serial.print("Battery Current: ");
  Serial.print(current);
  Serial.println(" mA");
  
  // Basic sensors
  bool wallDetected = roomba.isWallDetected(true);
  bool bumperPressed = roomba.isBumperPressed();
  
  Serial.print("Wall Detected: ");
  Serial.println(wallDetected ? "YES" : "NO");
  
  Serial.print("Bumper Pressed: ");
  Serial.println(bumperPressed ? "YES" : "NO");
  
  // Visual indicators with LEDs
  if (wallDetected) {
    roomba.setLED(false, true, false); // Spot LED on
  } else if (bumperPressed) {
    roomba.setLED(true, false, false); // Debris LED on
  } else {
    roomba.setLED(false, false, false); // All LEDs off
  }
  
  // Battery level indication with power LED
  if (voltage > 15000) {
    roomba.setPowerLED(0, 255);      // Green - good battery
  } else if (voltage > 13000) {
    roomba.setPowerLED(128, 255);    // Yellow - medium battery
  } else {
    roomba.setPowerLED(255, 255);    // Red - low battery
  }
  
  Serial.println("------------------------");
}