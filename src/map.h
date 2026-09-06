// map.h
#ifndef MAP_H
#define MAP_H
#include <vector>
#include <map>
#include "ArduRoomba.h"
#include <Arduino.h>

constexpr short NORMAL_SPEED = 350;
constexpr uint8_t TURN_SPEED = 250;
constexpr short OI_WEIGHT = 60;
constexpr short RESOLUTION = 330;
constexpr uint8_t angle = 90;
constexpr float factor = 0.005;
constexpr uint8_t range = 50;
constexpr uint8_t obstacleSize = 2;

using namespace std;

enum cellStatus : uint8_t {
  UNKNOWN = 0,
  FREE = 1,
  OBSTACLE = 2,
  DIRT = 3,
  PASSED = 4
};

enum states: uint8_t {
  SWEEPING = 0,
  BUMPED = 1,
  TURNING = 2,
  PATHFINDING = 3
};

enum directions: uint8_t {
  UP = 0,
  RIGHT = 1,
  DOWN = 2,
  LEFT = 3,
  STOP = 4
};


struct Point {
    short x, y;
    bool operator==(const Point& o) const { return x == o.x && y == o.y; }
};

template <typename T = uint8_t>
class mapRoom {

  private:
    vector<T> map;
    unsigned long lastTime;
    uint8_t dec;
    ArduRoomba &roomba;
    short height;
    short width;
    Point curr;
    states state;
    directions dir;
    uint8_t uTurnPhase;
    float currentAngle;

  public:
    mapRoom(ArduRoomba &roomba, const short h, const short w) : roomba(roomba), height(h), width(w), curr(), state(), dir(), uTurnPhase(0), currentAngle(0) {
      map.assign(height * width, UNKNOWN);
    }

    void setCell(const short x, const short y, const cellStatus status) {
      if ( x >= 0 && y >= 0 && x < width && y < height) {
        map[y * width + x] = status;
      }
    }

    uint8_t readCell(const short x, const short y) const {
      if ( x >= 0 && y >=0 && x < width && y < height) {
        return map[y * width + x];
      }
      return UNKNOWN;
    }

    void expandMap(const short newHeight, const short newWidth) {
      vector<T> newMap(newHeight * newWidth, UNKNOWN);

      for (short y = 0; y < height; ++y) {
        for (short x = 0; x < width; ++x) {
          newMap[y * newWidth + x] = map[y * width + x];
        }
      }

      map = move(newMap);
      width = newWidth;
      height = newHeight;
      Serial.println("Map expanded");
    }

    T& operator()(const short r, const short c) {
      return static_cast<T &>(map[r * width + c]);
    }

    const T& operator()(const short r,const short c) const {
      return static_cast<const T &>(map[r * width + c]);
    }

    static short mergeDistance(const short OIvalue, const short computed) {
      const short sum = (OIvalue * OI_WEIGHT) + (computed * ( 100 - OI_WEIGHT ));
      return sum / 100;
    }

    void start() {
      roomba.moveForward(NORMAL_SPEED);
      curr.x = 150;
      curr.y = 150;
      dec = 0;
      dir = UP;
      lastTime = millis();
    }

    void calculateCells(const bool sendCommand = true) {
      const float timePassed = (millis() - lastTime) / 1000.0; // millis to seconds
      const short distance = mergeDistance(roomba.getDistance(), static_cast<float>(NORMAL_SPEED) * timePassed); // get distance

      if ( distance >= RESOLUTION) {
        const float app = static_cast<float>(distance) / static_cast<float>(RESOLUTION); // total cells passed
        auto cells = static_cast<uint8_t>(app); // integer part
        dec += (static_cast<float>(cells) - app) * 100; // decimal part of the division

        if ( dec >= 100) { // if the remaining decimal is bigger than a cell add it to the map
          cells += 1;
          dec -= 100;
        }

        for (uint8_t i = 0; i < cells; ++i) { // set cells as passed
          if (dir == UP) curr.y += 1;
          else if (dir == LEFT) curr.x -= 1;
          else if (dir == DOWN) curr.y -= 1;
          else if (dir == RIGHT) curr.x += 1;
          setCell(curr.x, curr.y, PASSED);
        }

        if (sendCommand) { // when false this mean the function was called after a hit
          roomba.moveForward(NORMAL_SPEED); // resend command to reset roomba.getDistance
          lastTime = millis();
        }
      }
    }

    void setObstacle(const bool left) {
      short xAx = curr.x;
      if (left) { // add wall following
        for (uint8_t i = 0; i < obstacleSize; ++i) {
          setCell(xAx, curr.y, OBSTACLE);
          xAx += 1;
        }
      }
      else {
        for (uint8_t i = 0; i < obstacleSize; ++i) {
          setCell(xAx, curr.y, OBSTACLE);
          xAx -= 1;
        }
      }
    }

    // counter-clockwise angles are positives

    // start cleaning
    void clean() {
      bool left;
      switch (state) {
        case SWEEPING:
          if (collision(left)) {
            roomba.stop();
            calculateCells(false);
            setObstacle(left);
            currentAngle = 0;
            uTurnPhase = 0;
            state = TURNING;
            const short null = roomba.getAngle();
            roomba.spinLeft(TURN_SPEED);
          }
          calculateCells(true);
          break;

        case TURNING:
          short currAngle = roomba.getAngle();
          if (uTurnPhase == 0) {
            currentAngle += (static_cast<float>(currAngle) * factor);

            if ( abs(currentAngle) > static_cast<float>(currAngle) ) {
              const short null = roomba.getDistance(); // reset distance
              roomba.moveForward(NORMAL_SPEED);
              lastTime = millis();
              uTurnPhase = 1;
            }
          }
          if (uTurnPhase == 1) {
            const float timePassed = millis() - lastTime;
            const short distance = mergeDistance(roomba.getDistance(), NORMAL_SPEED * static_cast<short>(timePassed));
            const short difference = distance - RESOLUTION;

            if (difference > range) {
              roomba.stop();
              roomba.moveBackward(NORMAL_SPEED);
              vTaskDelay(pdMS_TO_TICKS(20));
            }
            else if (difference < -range) {
              roomba.stop();
              roomba.moveBackward(NORMAL_SPEED);
              vTaskDelay(pdMS_TO_TICKS(20));
            }

            roomba.stop();
            currentAngle = 0;
            uTurnPhase = 2;
            const short null = roomba.getAngle();
            roomba.spinRight(TURN_SPEED);
          }
          if (uTurnPhase == 2) {
            currAngle = roomba.getAngle();
            currentAngle  += static_cast<float>(currAngle) * factor;

            if ( abs(currentAngle) > static_cast<float>(angle) )  {
              const short nil = roomba.getDistance();
              roomba.moveForward(NORMAL_SPEED);
              lastTime = millis();
              uTurnPhase = 0;
              currAngle = 0;
              state = SWEEPING;

              if (dir == UP) {
                dir = DOWN;
              }
              else dir = UP;
              curr.x -= 1;
            }
          }

          break;

      }
    }

    bool collision(bool& left) const {
      const BumperData bumpers = roomba.readBumpers();
      if (bumpers.leftBumper) {
        left = true;
        return true;
      }
      if (bumpers.rightBumper) {
        left = false;
        return true;
      }
      return false;
    }


    // utility funcs
    short getRows() const { return height; }
    short getColumns() const { return width; }

    bool check(const short nx, const short ny, const bool countPassed = false) const {
      const uint8_t cell = readCell(nx, ny);
      return (cell == FREE) || (countPassed && cell == FREE);
    }

    directions getDirection(const short x, const short y, const bool countPassed = false) const {
      if (check(x + 1, y, countPassed)) return RIGHT;
      if (check(x - 1, y, countPassed)) return LEFT;
      if (check(x, y + 1, countPassed)) return UP;
      if (check(x, y - 1, countPassed)) return DOWN;
      return STOP;
    }


    // debug print
    void print() const {
      for (short y = 0; y < height; ++y) {
        for (short x = 0; x < width; ++x) {
          const uint8_t status = readCell(x, y);
          if (status == UNKNOWN) Serial.print(".");
          else if (status == FREE) Serial.print(" ");
          else if (status == DIRT) Serial.print("/");
          else if (status == OBSTACLE) Serial.print("X");
          else if (status == PASSED) Serial.print("0");
          else Serial.println("Error");
        }
      }
    }

    String getMapJSON() const {
      // Ottimizzazione memoria: inviamo solo le celle conosciute
      String json = "{\"w\":" + String(width) + ",\"h\":" + String(height) + 
                    ",\"cx\":" + String(curr.x) + ",\"cy\":" + String(curr.y) + ",\"cells\":[";
      
      bool first = true;
      for (short y = 0; y < height; ++y) {
        for (short x = 0; x < width; ++x) {
          uint8_t status = readCell(x, y);
          if (status != UNKNOWN) {
            if (!first) json += ",";
            // Formato compatto: [x, y, stato]
            json += "[" + String(x) + "," + String(y) + "," + String(status) + "]";
            first = false;
          }
        }
      }
      json += "]}";
      return json;
    }

};

vector<Point> findPath(const mapRoom<>& map, Point start, Point end);

#endif