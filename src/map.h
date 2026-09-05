// map.h
#ifndef MAP_H
#define MAP_H
#include <vector>
#include <set>
#include <map>
#include "ArduRoomba.h"
#include <Arduino.h>

#define NORMAL_SPEED = 300

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
    mapRoom(Arduroomba roomba, short h, short w) : roomba(roomba), height(h), width(w) {
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
      return (T&)map[r * width + c]; 
    }

    const T& operator()(short r, short c) const { 
      return (const T&)map[r * width + c]; 
    }

    void start() {
      roomba.moveForward();
    }


    // utility funcs
    short getRows() const { return height; }
    short getColumns() const { return width; }

    bool check(short nx, short ny, bool countPassed = false) {
      uint8_t cell = readCell(nx, ny);
      return (cell == FREE) || (countPassed && cell == FREE);
    }

    directions getDirection(short x, short y, bool countPassed = false) {
      if (check(x + 1, y)) return RIGHT;
      else if (check(x - 1, y)) return LEFT;
      else if (check(x, y + 1)) return UP;
      else if (check(x, y - 1)) return DOWN;
      else return STOP;
    }


    // debug print
    void print() {
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