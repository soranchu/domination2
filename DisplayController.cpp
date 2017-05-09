#include "DisplayController.h"
#include "mbed.h"
#include "ssd1331.h"

DisplayController::DisplayController (
    PinName cs, PinName rst, PinName mode, 
    PinName mosi, PinName miso, PinName sclk) {

  oled = new ssd1331(cs, rst, mode, mosi, miso, sclk);
  oled->set_font(NULL);
  tick = 0;
}

void DisplayController::update() {
  oled->locate(89, 56);
  if (status->buttons[GameStatus::TeamR]) {
    oled->foreground(0b1111100000000000);
    oled->printf("*");
  } else {
    oled->printf(" ");
  }
  oled->locate(0, 56);
  if (status->buttons[GameStatus::TeamY]) {
    oled->foreground(0b1111111111100000);
    oled->printf("*");
  } else {
    oled->printf(" ");
  }

  oled->foreground(0xffff);
  oled->locate(0, 8);
  oled->printf("%2d", status->cfgSense);
//  oled->printf("%02X %02X", status->nodeId, status->current);


  oled->locate(0, 0);
  uint32_t seconds = status->clock / 5;
  uint8_t minute = seconds / 60;
  uint8_t sec = seconds % 60;

  if (status->clock % 5 == 0) {
    tick = !tick;
  }

  if (tick) {
    //ioxp->gpioPort(0b00110000);
    oled->printf("%02d:%02d ", minute, sec);
  } else {
    //ioxp->gpioPort(0b00000000);
    oled->printf("%02d %02d ", minute, sec);
  }

  oled->foreground(0b1111100000000000);
  oled->printf("R:%02d ", status->teams[GameStatus::TeamR].detectedTags);

  oled->foreground(0b1111111111100000);
  oled->printf("Y:%02d", status->teams[GameStatus::TeamY].detectedTags);
}

void DisplayController::setGameStatus(GameStatus* status) {
  this->status = status;
}