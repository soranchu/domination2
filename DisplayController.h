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
private:
  ssd1331* oled;
  GameStatus* status;
  uint8_t tick;
};

#endif