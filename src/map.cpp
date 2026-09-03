#include <vector>
#include <algorithm>
#include <queue>
#include "map.h"

using namespace std;

// Mapping
#define RESOLUTION 10 // cm for map

const uint8_t START_MARK = 28; // 281 is too big
const short dx[] = {1, -1, 0, 0};
const short dy[] = {0, 0, -1, 1};


vector<Point> findPath(const mapRoom<uint8_t>& map, Point start, Point end) {
  short width = map.getColumns();
  short height = map.getRows();

  vector<uint8_t> from(width * height, UNKNOWN);

  queue<Point> q;
  q.push(start);

  from[start.y * width + start.x] = START_MARK;

  bool trovato = false;

  while(!q.empty()) {
    Point curr = q.front();
    q.pop();

    if (curr == end) {
      trovato = true;
      break;
    }

    for (uint8_t i = 0; i < 4; ++i) {
      short nx = curr.x + dx[i];
      short ny = curr.y + dy[i];

      if (nx >= 0 && nx < width && ny >= 0 && ny < height ) {
        int index = ny * width + nx;
        uint8_t status = map.readCell(nx, ny); 

        if (from[index] == 255 && (status == FREE || status == DIRT || status == PASSED)) {
          from[index] = i;
          q.push({nx, ny});
        }
      }
    }
  }

  if (!trovato) return {};

  vector<Point> path;
  Point curr = end;

  while(!(curr == start)) {
    path.push_back(curr);
    uint8_t dir = from[curr.y * width + curr.x];
    curr.x -= dx[dir];
    curr.y -= dy[dir];
  }

  path.push_back(start);

  reverse(path.begin(), path.end());

  return path;
}

