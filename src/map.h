// map.h
#ifndef MAP_H
#define MAP_H
#include <vector>
#include <map>
#include "ArduRoomba.h"
#include <Arduino.h>

constexpr uint8_t NORMAL_SPEED = 300;
constexpr uint8_t TURN_SPEED = 200;
constexpr short OI_WEIGHT = 60;

using namespace std;

enum cellStatus : uint8_t {
  UNKNOWN = 0,
  FREE = 1,
  OBSTACLE = 2,
  DIRT = 3,
  PASSED = 4
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
    vector<uint8_t> map;
    ArduRoomba roomba;
    short height;
    short width;

  public:
    mapRoom(const ArduRoomba &roomba, const short h, const short w) : roomba(roomba), height(h), width(w) {
      map.assign(height * width, UNKNOWN);
    }

    void setCell(short x, short y, cellStatus status) {
      if ( x >= 0 && y >= 0 && x < width && y < height) {
        map[y * width + x] = status;
      }
    }

    uint8_t readCell(short x, short y) const {
      if ( x >= 0 && y >=0 && x < width && y < height) {
        return map[y * width + x];
      }
      return UNKNOWN;
    }

    void expandMap(short newHeight, short newWidth) {
      vector<uint8_t> newMap(newHeight * newWidth, UNKNOWN);

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

    T& operator()(short r, short c) { 
      return static_cast<T &>(map[r * width + c]);
    }

    const T& operator()(const short r,const short c) const {
      return static_cast<const T &>(map[r * width + c]);
    }

    short mergeDistance(const short OIvalue, const short computed) {
      const short sum = (OIvalue * OI_WEIGHT) + (computed * ( 100 - OI_WEIGHT ));
      return sum / 100;
    }

    void start() {
      const unsigned long startTime = millis();
      roomba.moveForward(NORMAL_SPEED);
      if (collision()) {
        const unsigned long time = millis() - startTime;
        short distance = mergeDistance(roomba.getDistance(), time);
      }

    }

    bool collision() {
      const BumperData bumpers = roomba.sensors().readBumpers();
      if (bumpers.leftBumper) {
        roomba.stop();
        roomba.turnLeft(TURN_SPEED);
      }
    }


    // utility funcs
    short getRows() const { return height; }
    short getColumns() const { return width; }

    bool check(const short nx, const short ny, const bool countPassed = false) const {
      const uint8_t cell = readCell(nx, ny);
      return (cell == FREE) || (countPassed && cell == FREE);
    }

    directions getDirection(const short x, const short y, bool countPassed = false) const {
      if (check(x + 1, y)) return RIGHT;
      else if (check(x - 1, y)) return LEFT;
      else if (check(x, y + 1)) return UP;
      else if (check(x, y - 1)) return DOWN;
      else return STOP;
    }


    // debug print
    void print() const {
      for (short y = 0; y < height; ++y) {
        for (short x = 0; x < width; ++x) {
          uint8_t status = readCell(x, y);
          if (status == UNKNOWN) Serial.print(".");
          else if (status == FREE) Serial.print(" ");
          else if (status == DIRT) Serial.print("/");
          else if (status == OBSTACLE) Serial.print("X");
          else if (status == PASSED) Serial.print("0");
          else Serial.println("Error");
        }
      }
    }

};

vector<Point> findPath(const mapRoom<uint8_t>& map, Point start, Point end);

#endif