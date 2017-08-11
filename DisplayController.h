#ifndef _DISPLAY_CONTROLLER_H_
#define _DISPLAY_CONTROLLER_H_

#include <inttypes.h>
#include "GameStatus.h"
#include "ssd1331.h"

class DisplayController {
public:
  DisplayController(
    PinName cs, PinName rst, PinName mode, 
    PinName mosi, PinName miso, PinName sclk);
  void update(bool fullUpdate);
  void setGameStatus(GameStatus* status);
  void debug(const char* msg);
  void debugHex(const char* msg, uint8_t len);
private:
  ssd1331* oled;
  GameStatus* status;
  char debugMessage[16];
  uint8_t tick;
};

#endif