# ArduRoomba - Clean & Minimal

A minimal, focused Arduino library for iRobot Open Interface communication.

## Design Philosophy

- **Minimal Core**: Essential functionality only
- **Smart Modularity**: Clean interfaces for future extensions
- **No Bloat**: Simple, readable code without over-engineering
- **Extensible**: Easy to add WiFi, Bluetooth, RTOS when needed

## Architecture

```
ArduRoomba/
├── src/
│   ├── ArduRoomba.h/.cpp          # Main library interface
│   └── extensions/                # Future: WiFi, Bluetooth, etc.
└── examples/
    ├── BasicMovement/
    ├── SensorReading/
    └── RemoteControl/
```

## Core Features

- iRobot Open Interface protocol implementation
- Basic movement commands (drive, turn, stop)
- Sensor reading (individual and streaming)
- Simple LED and sound control
- Clean error handling (bool returns, no complex enums)

## Future Extensions

- WiFi module (ESP32/ESP8266)
- Bluetooth control
- RTOS integration
- Advanced sensors
- Custom behaviors

## Usage

```cpp
#include "ArduRoomba.h"

ArduRoomba roomba(2, 3, 4); // RX, TX, BRC pins

void setup() {
  Serial.begin(19200);
  if (roomba.begin()) {
    Serial.println("Roomba connected!");
    roomba.moveForward();
    delay(2000);
    roomba.stop();
  }
}
```

## Supported Hardware

- Arduino Uno R3/R4
- ESP32/ESP8266
- iRobot Create 2, Roomba 500/600/700 series