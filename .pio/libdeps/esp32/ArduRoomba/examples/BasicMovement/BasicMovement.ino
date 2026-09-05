/**
 * BasicMovement.ino
 * 
 * Simple example showing basic movement commands with the ArduRoomba library.
 * Demonstrates forward/backward movement, turning, and stopping.
 */

#include "ArduRoomba.h"

// Pin connections to Roomba
// RX Pin (Roomba TX) -> Pin 2  
// TX Pin (Roomba RX) -> Pin 3
// BRC Pin (Roomba DD) -> Pin 4
ArduRoomba roomba(2, 3, 4);

void setup() {
  Serial.begin(19200);
  
  // Enable debug output
  roomba.setDebug(true);
  
  // Initialize connection to Roomba
  if (roomba.begin()) {
    Serial.println("Roomba connected successfully!");
    
    // Run movement sequence
    movementDemo();
    
  } else {
    Serial.println("Failed to connect to Roomba!");
    Serial.println("Check wiring and power.");
  }
}

void loop() {
  // Nothing to do in main loop for this example
}

void movementDemo() {
  Serial.println("\n=== Starting Movement Demo ===");
  
  // Move forward for 2 seconds
  Serial.println("Moving forward...");
  roomba.moveForward(200);  // 200 mm/s
  delay(2000);
  
  // Stop
  Serial.println("Stopping...");
  roomba.stop();
  delay(1000);
  
  // Turn right for 1 second
  Serial.println("Turning right...");
  roomba.turnRight(150);
  delay(1000);
  
  // Stop
  roomba.stop();
  delay(1000);
  
  // Move backward for 1.5 seconds
  Serial.println("Moving backward...");
  roomba.moveBackward(150);
  delay(1500);
  
  // Stop
  roomba.stop();
  delay(1000);
  
  // Turn left for 1 second
  Serial.println("Turning left...");
  roomba.turnLeft(150);
  delay(1000);
  
  // Final stop
  Serial.println("Final stop");
  roomba.stop();
  
  // Beep to indicate completion
  roomba.beep();
  
  Serial.println("=== Movement Demo Complete ===");
}