#include "ArduRoomba.h" 
// #include "/../../libraries/ArduRoomba/src/extensions/ArduRoombaESP32WiFi.h" LOL
#include "extensions/ArduRoombaESP32WiFi.h"
#include "map.h"
//#include "extensions/ArduRoombaBLE.h"
#include <vector>
#include <queue>
#include <algorithm>

using namespace std;

// TODO remove overcurrent in updateSafety and use it to check if it found a smal obstacle
// TODO remove bumper detection in updateSafety and fix cliff detection
// TODO implement mapping to show on a canvas in the web server with Spannig Tree Coverage
// TODO implement simple path finding to improve dock function (WaveFront alg)
// TODO imlement dock and spot function
// TODO se dopo tot tempo non trova la base fermati e fai una melodia
// TODO se trovi dirt detect allora guarda intorno e vedi se ne trovi altro
// TODO fotoresistenze per non fermarsi sotto i divani
// TODO IR receiver/transmitter per individuare ostacoli
// TODO PID per controllo direzione (magari anche con MPU-608)
// TODO analisi euristica dei punti sporchi
// TODO STEALTH mode
// TODO sensore di suono per attivazione remota
// TODO LittleFS library for saving and loading the maps
// TODO timer con priorità tramite FreeRTOS per le varie task 
// /TODO add BLE ???

#define PWD "Qwert128mz"
#define LOW_BATT 14000
#define CRITICAL_BATT 13000 // standard for LiPO
#define SSID "Roomba"
//#define BLEID "Roomba"
#define LED_PIN 2
#define RATE 50 // rate limiting for netwrok commands

// DIRT Threshold
#define DIRT_MEDIUM 100
#define DIRT_HIGH 175 

// cycles
#define SPIRAL_MEDIUM 5
#define SPIRAL_BIG 10
#define SPIRAL_SMALL 3 

// Spiral
#define RAD_START 50 // radius star for the spiral
#define SPIRAL_DELAY 1 // time necessary to do a spiral (at first)
#define SPIRAL_SPEED 200 // speed when doing a spiral

#define NORMAL_SPEED 350
#define SLOW_SPEED 150
#define HIGH_SPEED 450 // high speed ig
#define RAD_INCREASE 50 // radius increase in a spiral
#define SPEED_BEFORE_HIT 50 // self.explanatory
#define BACKWARD_SPEED 100 // speed after hit
#define COLLISION_DELAY 50 // polling delay for collision()


RoombaConfig config = RoombaConfig::createESP32(&Serial2, 5);
ArduRoomba roomba(config);

ArduRoombaESP32WiFi wifi(roomba);
//ArduRoombaBLE ble(roomba, BLEID);


void setup() {
  // Configura la seriale di debug per ESP32 (Inserito 115200)
  Serial.begin(115200);
  while(!Serial);

  // Configura la seriale hardware per il Roomba (Inizializzata una sola volta)
  Serial2.begin(19200, SERIAL_8N1, 16, 17);

  // Configurazione dei parametri (Aggiunti i punti e virgola ';')
  config.lowBatteryThreshold = LOW_BATT;
  config.criticalBatteryThreshold = CRITICAL_BATT;
  config.enableSafety = true;

  // Inizializzazione della libreria Roomba
  roomba.setDebug(true);
  roomba.enableSafety(true);


  if(roomba.begin(115200)){
    if (!wifi.beginAP(SSID, PWD)) {
      Serial.println("ERROR: Failed to create WiFi AP!");
      errorBlink();
      while (1) delay(100);
    }
    Serial.println("✓ WiFi AP created!");
    wifi.startWebServer(80);
    wifi.setLowBatteryThreshold(LOW_BATT);


    Serial.print("Control at http://");
    Serial.println(wifi.getIPAddress());


   /* if(!ble.begin()){
      Serial.println("Failed to start BLE");
      errorBlink();
      while (1) delay(100);
    }*/

    //Serial.println("Bluetooth started");

    Serial.println("\n=== Setup Complete ===");

    roomba.actuators().playStartupSong();
  }
  else Serial.println("Failed to start roomba, chech wirings");

}

void loop() {
  unsigned long int startTime = millis();
  while (millis() - startTime < 10000) {
    roomba.updateSafety(); // Safety checks every loop iteration
    delay(50);
  }
  ButtonData buttons = roomba.sensors().readButtons();
  if (buttons.clean){
    clean();
  }
  else if(buttons.spot){
    spot();
  }
  else if (buttons.dock){
    dock();
  }
  network();
}

void dock(){

}

void spot(){
  
}

// ================================= CLEAN ====================================

void collision(bool walling = false){ // TODO add IR and finish collision system
    if (roomba.isConnected() ) {
        if (roomba.isWallDetected(true) ) {
          roomba.moveForward(SPEED_BEFORE_HIT);
        }
        if (roomba.isObstacle() ) {
          roomba.moveForward(SPEED_BEFORE_HIT);
          delay(50);
          if (roomba.isObstacle()) {
            roomba.moveBackward(BACKWARD_SPEED);
          }
        }
    }
}

void clean(){
  if(roomba.isConnected()){
    spiraling(SPIRAL_MEDIUM);
    forward();
  }
}

void forward(){
  unsigned long int last = millis();

  roomba.moveForward(NORMAL_SPEED);
  while (true) { // TODO check if it finished
    if (last - millis() > COLLISION_DELAY) {
      collision(true);
    }
  }
}

void spiraling(uint8_t circles){
    uint8_t radius = RAD_START;
    uint8_t speed = SPIRAL_SPEED;
    uint8_t delay = SPIRAL_DELAY;
    unsigned long int last = millis();

    roomba.movement().drive(speed, radius);

    while(circles > 0){
        collision();
        if(millis() - last > delay){
            radius+=RAD_INCREASE;
            delay+=(RAD_INCREASE/10)*3;
            last = millis();
            roomba.movement().drive(speed, radius);
            --circles;
        }
    }
    
}

bool dirtMed(uint8_t dirt){
  return (dirt > DIRT_MEDIUM) ? true : false;
}

bool dirtHigh(uint8_t dirt){
  return (dirt > DIRT_HIGH) ? true : false;
}

void network(){
  static unsigned long lastCommandTime = 0;
  static unsigned long lastSafetyCheck = 0;

  wifi.handleClient();
  //ble.updateStatus();

  if(millis() - lastCommandTime > RATE){
    return;
  }

  if (millis() - lastSafetyCheck > 500) {
    lastSafetyCheck = millis();
    safetyCheck();
  }

}

void safetyCheck() {
  // Check battery status
  uint16_t voltage = roomba.getBatteryVoltage();
  static bool wasLow = false;

  if (voltage > 0 && voltage < 12000) {
    if (!wasLow) {
      Serial.println("WARNING: Battery critically low!");
      roomba.actuators().playAlertSong();
      wifi.enableRemoteControl(false); // Disable remote commands
      //ble.enableRemoteControl(false);
      digitalWrite(LED_PIN, LOW);
      wasLow = true;
    }
  } else if (voltage > 0 && voltage < 13500) {
    static unsigned long lastWarning = 0;
    if (millis() - lastWarning > 30000) { // Warn every 30s
      Serial.println("WARNING: Battery low");
      lastWarning = millis();
    }
  } else {
    wasLow = false;
    wifi.enableRemoteControl(true);
    //ble.enableRemoteControl(true);
    digitalWrite(LED_PIN, HIGH);
  }

  // Optional: Automatic obstacle avoidance when idle
  // Uncomment to enable:
  
  if (!wifi.isRemoteEnabled()) {
    if (roomba.isBumperPressed()) {
      Serial.println("Auto-avoidance: Bumper hit");
      roomba.moveBackward(150);
      delay(500);
      roomba.spinRight(150);
      delay(500);
      roomba.stop();
    }
  }
}

void errorBlink() {
  while (true) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}

void status(){
  Serial.println("\n--- Status ---");
  Serial.print("BLE Connected: ");
  //Serial.println(ble.isConnected() ? "Yes" : "No");
  Serial.print("Connection Count: ");
  //Serial.println(ble.getConnectionCount());
  Serial.print("Battery Voltage: ");
  Serial.print(roomba.getBatteryVoltage());
  Serial.println(" mV");
  Serial.print("Battery Percent: ");
  Serial.print(roomba.getBatteryPercent());
  Serial.println("%");
  Serial.print("Wall Detected: ");
  Serial.println(roomba.isWallDetected(false) ? "Yes" : "No");
  Serial.print("Bumper Pressed: ");
  Serial.println(roomba.isBumperPressed() ? "Yes" : "No");
}