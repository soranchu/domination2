#ifndef _GAME_STATUS_H_
#define _GAME_STATUS_H_
#include <inttypes.h>

struct GameStatus {
  enum Team {
    TeamR = 0,
    TeamY = 1
  };
  struct TeamStatus {
    uint16_t totalPoint;
    uint16_t point;
    uint8_t tags;
    uint8_t button;
  };
  uint32_t clock;
  uint32_t totalTime;
  TeamStatus teams[2];
  uint8_t progress;
  bool decreasing;
  uint8_t current;
  uint8_t attacker;
  bool started;
  uint8_t nodeId;
  int8_t cfgSense;

  void reset() {
    clock = 60 * 7 * 5;
    totalTime = 0;
    for(uint8_t i = 0; i < 2; ++i) {
      teams[i].totalPoint = 0;
      teams[i].point = 0;
      teams[i].tags = 0;
      teams[i].button = 0;
    }
    progress = 0;
    decreasing = false;
    current = 0xff;
    attacker = 0xff;
    started = false;
  }

  void increaseProgress(uint8_t val) {
    progress += val;
    decreasing = false;
  }
  void resetProgress() {
    progress = 0;
    decreasing = false;
  }
  void decreaseProgress() {
    if (progress > 2) {
      progress -= 2;
      decreasing = true;
    } else {
      progress = 0;
      decreasing = false;
    }
  }
};

#endif