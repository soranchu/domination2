#include "DisplayController.h"
#include "mbed.h"
#include "ssd1331.h"
static uint16_t COLOR_RED = 0b1111100000000000;
static uint16_t COLOR_YEL = 0b1111111111100000;
DisplayController::DisplayController (
    PinName cs, PinName rst, PinName mode, 
    PinName mosi, PinName miso, PinName sclk) {

  oled = new ssd1331(cs, rst, mode, mosi, miso, sclk);
  oled->set_font(NULL);
  tick = 0;
}

void DisplayController::update() {
  oled->locate(89, 56);
  if (status->teams[GameStatus::TeamR].button) {
    oled->foreground(COLOR_RED);
    oled->printf("*");
  } else {
    oled->printf(" ");
  }
  oled->locate(0, 56);
  if (status->teams[GameStatus::TeamY].button) {
    oled->foreground(COLOR_YEL);
    oled->printf("*");
  } else {
    oled->printf(" ");
  }

  oled->locate((uint8_t)(6*6.5), 8);
  if (status->progress != 0) {
    oled->foreground(status->attacker == GameStatus::TeamR ? COLOR_RED : COLOR_YEL);
    oled->printf("%02d", status->progress);
  } else {
    oled->printf("  ");    
  }
//  oled->printf("%02X %02X", status->nodeId, status->current);


  oled->locate(0, 0);
  uint32_t seconds = status->clock / 5;
  uint8_t minute = seconds / 60;
  uint8_t sec = seconds % 60;

  if (status->clock % 5 == 0) {
    tick = !tick;
  }

  oled->foreground(0xffff);
  oled->locate(6*5, 0);
  if (tick) {
    oled->printf("%02d:%02d", minute, sec);
  } else {
    oled->printf("%02d %02d", minute, sec);
  }

  oled->locate(0, 8);
  oled->foreground(COLOR_RED);
  oled->printf("%02d", status->teams[GameStatus::TeamR].tags);

  oled->locate(6*13, 8);
  oled->foreground(COLOR_YEL);
  oled->printf("%02d", status->teams[GameStatus::TeamY].tags);

  oled->locate(0, 0);
  oled->foreground(COLOR_RED);
  oled->printf("%03d", status->teams[GameStatus::TeamR].point/5);

  oled->locate(6*12, 0);
  oled->foreground(COLOR_YEL);
  oled->printf("%03d", status->teams[GameStatus::TeamY].point/5);
}

void DisplayController::setGameStatus(GameStatus* status) {
  this->status = status;
}