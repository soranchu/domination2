#ifndef _GAME_STATUS_H_
#define _GAME_STATUS_H_
#include <inttypes.h>

struct GameStatus {
  enum Teams {
    TeamR = 0,
    TeamY = 1
  };
  struct TeamStatus {
    uint16_t totalPoint;
    uint16_t currentPoint;
    uint8_t detectedTags;
  };
  uint32_t clock;
  uint32_t totalTime;
  TeamStatus teams[2];
  uint8_t progress;
  uint8_t current;
  uint8_t state;
  uint8_t nodeId;
  uint8_t buttons[2];
  int8_t cfgSense;
};

#endif