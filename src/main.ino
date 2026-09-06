

#include "ArduRoomba.h" 
// #include "/../../libraries/ArduRoomba/src/extensions/ArduRoombaESP32WiFi.h" LOL
#include "extensions/ArduRoombaESP32WiFi.h"
#include "map.h"
//#include "extensions/ArduRoombaBLE.h"

using namespace std;

// TODO: remove overcurrent in updateSafety and use it to check if it found a small obstacle
// TODO: remove bumper detection in updateSafety and fix cliff detection
// TODO: implement mapping to show on a canvas in the web server with Spanning Tree Coverage
// TODO: implement simple path finding to improve dock function (WaveFront alg)
// TODO: implement dock and spot function
// TODO: se dopo tot tempo non trova la base fermati e fai una melodia
// TODO: se trovi dirt detect allora guarda intorno e vedi se ne trovi altro
// TODO: fotoresistenze per non fermarsi sotto i divani
// TODO: IR receiver/transmitter per individuare ostacoli
// TODO: PID per controllo direzione (magari anche con MPU-608)
// TODO: analisi euristica dei punti sporchi
// TODO: STEALTH mode
// TODO: sensore di suono per attivazione remota
// TODO: LittleFS library for saving and loading the maps
// TODO: timer con priorità tramite FreeRTOS per le varie task
// /TODO add BLE ???

#define NETWORK_RATE 20
#define USE_AP_MODE false
#define AP_SSID "MY-WIFI-LTE"
#define PWD "Qwert128mz"
#define LOW_BATT 14000
#define CRITICAL_BATT 13000 // standard for LiPO
#define SSID "Roomba"
//#define BLEID "Roomba"
#define LED_PIN 2
#define RATE 50 // rate limiting for network commands

// DIRT Threshold
#define DIRT_MEDIUM 100
#define DIRT_HIGH 175 

//TASK
#define STACK_CLEAN 8192
#define STACK_NORMAL 4196
#define STACK_NETWORK 4196

// cycles
#define SPIRAL_MEDIUM 5
#define SPIRAL_BIG 10
#define SPIRAL_SMALL 3 

// Spiral
#define RAD_START 50 // radius star for the spiral
#define SPIRAL_DELAY 1 // time necessary to do a spiral (at first)
#define SPIRAL_SPEED 200 // speed when doing a spiral

#define SLOW_SPEED 150
#define HIGH_SPEED 450 // high speed ig
#define RAD_INCREASE 50 // radius increase in a spiral
#define SPEED_BEFORE_HIT 50 // self.explanatory
#define BACKWARD_SPEED 100 // speed after hit
#define COLLISION_DELAY 50 // polling delay for collision()

// Map
#define HEIGHT 300
#define WIDTH 300


RoombaConfig static config = RoombaConfig::createESP32(&Serial2, 5);
ArduRoomba static roomba(config);

ArduRoombaESP32WiFi static wifi(roomba);
//ArduRoombaBLE ble(roomba, BLEID);#include <Arduino.h>

TaskHandle_t static xNormal = nullptr;
TaskHandle_t static xClean = nullptr;
TaskHandle_t static xNetwork = nullptr;

constexpr unsigned long F1 = 10;
constexpr unsigned long F2 = 20;
constexpr unsigned long F3 = 50;
constexpr unsigned long F4 = 100;
constexpr unsigned long F5 = 500;
constexpr unsigned long F6 = 1000;

static void clean(void *pvParameters);
static void normal(void *pvParameters);
static void taskNetwork(void *pvParameters);
static void status();
static void spiraling(uint8_t circles);
static void errorBlink();
static void startTask();
static void network();
static void startWifi();
static bool dirtHigh(uint8_t dirt);
static bool dirtMed(uint8_t dirt);
static void spot();
static void dock();
static void buttons();


void setup() {

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
        startWifi();


    /* if(!ble.begin()){
        Serial.println("Failed to start BLE");
        errorBlink();
        while (1) delay(100);
        }*/

        //Serial.println("Bluetooth started");

        Serial.println("\n=== Setup Complete ===");

        roomba.actuators().playStartupSong();

        startTask();
    }
    else Serial.println("Failed to start roomba, chech wirings");

}


void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}

// ============================== NETWORK ==================================

static void startWifi() {
    #if USE_AP_MODE
            // Access Point mode
            if (!wifi.beginAP(SSID, PWD)) {
                Serial.println("ERROR: Failed to create WiFi AP!");
                errorBlink();
                while (true) delay(100);
            }
            Serial.println("✓ WiFi AP created!");
        #else
            // Client mode
            if (!wifi.beginClient(AP_SSID, PWD)) {
                Serial.println("ERROR: Failed to connect to WiFi!");
                errorBlink();
                while (true) delay(100);
            }
            Serial.println("✓ WiFi connected!");
        #endif

     // Start web server
        wifi.startWebServer(80);
        wifi.setLowBatteryThreshold(12000); // Stop commands below 12V

        Serial.println("\n=== Setup Complete ===");
        #if USE_AP_MODE
            Serial.println("Connect to WiFi network:");
            Serial.print("  SSID: ");
            Serial.println(AP_SSID);
            if (PWD) {
                Serial.print("  Password: ");
                Serial.println(PWD);
            }
        #endif
            Serial.println("\nOpen browser to:");
            Serial.print("  http://");
            Serial.println(wifi.getIPAddress());

        #if !USE_AP_MODE
            Serial.print("  Signal Strength: ");
            Serial.print(wifi.getRSSI());
            Serial.println(" dBm");
        #endif
}

static void network(){

  wifi.handleClient();
  //ble.updateStatus();

  /*if(millis() - lastCommandTime > RATE){
    return;
  }*/

}

// ============================== TASKS =================================


static void startTask() {
    xTaskCreatePinnedToCore(
            normal,
            "normal",
            STACK_NORMAL,
            nullptr,
            3,
            &xNormal,
            1
        );
    xTaskCreatePinnedToCore(
            taskNetwork,
            "network",
            STACK_NETWORK,
            nullptr,
            3,
            &xNetwork,
            0
        );
}

void normal(void* pvParameters) {
    constexpr TickType_t xFrequency = pdMS_TO_TICKS(1);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    unsigned long lastF1 = 0, lastF2 = 0, lastF3 = 0;
    unsigned long lastF4 = 0, lastF5 = 0, lastF6 = 0;

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        const unsigned long time = millis();

        if (time - lastF1 >= F1) {
        lastF1 = time;
            roomba.updateSafety();
        }
        
        if (time - lastF2 >= F2) {
        lastF2 = time;
            network();
        }

        if (time - lastF3 >= F3) {
        lastF3 = time;
            buttons();
        }
    }
}


static void deleteTask(TaskHandle_t& task) {
    if (task != nullptr) {
        vTaskDelete(task);
        task = nullptr;
    }
}

void taskNetwork(void *pvParameters) {
    constexpr TickType_t xFrequency = pdMS_TO_TICKS(NETWORK_RATE);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        network();
    }
}

static void switchClean() {
    deleteTask(xNormal);
    xTaskCreatePinnedToCore(
        clean,
        "clean",
        STACK_CLEAN,
        nullptr,
        3,
        &xClean,
        1
    );
}

static void buttons() {
    const ButtonData buttons = roomba.sensors().readButtons();
    if (buttons.clean){
        switchClean();
        clean(nullptr);
    }
    else if(buttons.spot){
        spot();
    }
    else if (buttons.dock){
        dock();
    }
}

static void spot() {
    return;
}

static void dock() {
    return;
}

void clean(void* pvParameters) {
    constexpr TickType_t xFrequency = pdMS_TO_TICKS(1);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    unsigned long lastF1 = 0, lastF2 = 0, lastF3 = 0;
    unsigned long lastF4 = 0, lastF5 = 0, lastF6 = 0;

    mapRoom<> map(roomba, HEIGHT, WIDTH);
    map.start();


    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        const unsigned long time = millis();

        if (time - lastF1 >= F1) {
        lastF1 = time;
            roomba.updateSafety();
        }

        if (time - lastF2 >= F2) {
        lastF2 = time;

        }

        if (time - lastF3 >= F3) {
        lastF3 = time;
            map.clean();
        }

        if (time - lastF4 >= F4) {
        lastF4 = time;
        
        }

        if (time - lastF5 >= F5) {
        lastF5 = time;
        
        }

        if (time - lastF6 >= F6) {
        lastF6 = time;
        
        }
    }
}

// ============================= CLEAN ========================================

/*static void collision(bool walling = false){ // TODO add IR and finish collision system
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

static void clean(){
  if(roomba.isConnected()){
    spiraling(SPIRAL_MEDIUM);
    forward();
  }
}

static void forward(){
  unsigned long last = millis();

  roomba.moveForward(NORMAL_SPEED);
  while (true) { // TODO check if it finished
    if (last - millis() > COLLISION_DELAY) {
      collision(true);
    }
  }
}

static void spiraling(uint8_t circles){
    uint8_t radius = RAD_START;
    uint8_t speed = SPIRAL_SPEED;
    uint8_t delay = SPIRAL_DELAY;
    unsigned long last = millis();

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

static bool dirtMed(const uint8_t dirt){
  return (dirt > DIRT_MEDIUM) ? true : false;
}

static bool dirtHigh(const uint8_t dirt){
  return (dirt > DIRT_HIGH) ? true : false;
}
*/
// ================================ SAFETY =========================

static void safetyCheck() {
  // Check battery status
  const uint16_t voltage = roomba.getBatteryVoltage();
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

// =========================== DEBUG =====================================

static void errorBlink() {
  while (true) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}

static void status(){
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