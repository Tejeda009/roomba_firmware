

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

#define USE_AP_MODE false
#define AP_SSID "MY-WIFI-LTE"
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
//ArduRoombaBLE ble(roomba, BLEID);#include <Arduino.h>

const unsigned long F1 = 10;
const unsigned long F2 = 20;
const unsigned long F3 = 50;
const unsigned long F4 = 100;
const unsigned long F5 = 500;
const unsigned long F6 = 1000;

void scheduler(void *pvParameters);

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
        #if USE_AP_MODE
            // Access Point mode
            if (!wifi.beginAP(SSID, PWD)) {
                Serial.println("ERROR: Failed to create WiFi AP!");
                errorBlink();
                while (1) delay(100);
            }
            Serial.println("✓ WiFi AP created!");
        #else
            // Client mode
            if (!wifi.beginClient(AP_SSID, PWD)) {
                Serial.println("ERROR: Failed to connect to WiFi!");
                errorBlink();
                while (1) delay(100);
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


    /* if(!ble.begin()){
        Serial.println("Failed to start BLE");
        errorBlink();
        while (1) delay(100);
        }*/

        //Serial.println("Bluetooth started");

        Serial.println("\n=== Setup Complete ===");

        roomba.actuators().playStartupSong();

        xTaskCreatePinnedToCore(
            scheduler,
            "Scheduler",
            8192,
            NULL,
            3,
            NULL,
            1
        );
    }
    else Serial.println("Failed to start roomba, chech wirings");

}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
}

void scheduler(void * pvParamenters) {
    TickType_t xFrequency = pdMS_TO_TICKS(1);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    unsigned long lastF1 = 0, lastF2 = 0, lastF3 = 0;
    unsigned long lastF4 = 0, lastF5 = 0, lastF6 = 0;


    for (;;) {
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
        unsigned long time = millis();

        if (time - lastF1 >= F1) {
        lastF1 = time;
       
        }

        if (time - lastF2 >= F2) {
        lastF2 = time;
        
        }

        if (time - lastF3 >= F3) {
        lastF3 = time;
        
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
