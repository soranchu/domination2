#include "DisplayController.h"
#include "mbed.h"
#include "ssd1331.h"
static uint16_t COLOR_RED = 0b1111100000000000;
static uint16_t COLOR_YEL = 0b1111111111100000;
static uint16_t COLOR_GL =  0b0000100000100001;
static uint16_t COLOR_LGL = 0b0001100001100011;
DisplayController::DisplayController (
    PinName cs, PinName rst, PinName mode, 
    PinName mosi, PinName miso, PinName sclk) {

  oled = new ssd1331(cs, rst, mode, mosi, miso, sclk);
  oled->set_font(NULL);
  tick = 0;
  this->debugMessage[0] = '\0';
}

void DisplayController::update(bool fullUpdate) {
  // draw progress
  uint8_t margin = 4;
  uint8_t top = 16;
  uint8_t start = margin + 2; 
  uint8_t w = width - margin - 2 - start;
  uint16_t col_cur = 
    status->current == 0xff ? COLOR_GL :
    status->current == GameStatus::TeamR ? COLOR_RED : COLOR_YEL;
  if (status->progress != 0) {
    uint16_t col_att = 
      status->decreasing ? COLOR_LGL : 
      status->attacker == GameStatus::TeamR ? COLOR_RED : COLOR_YEL;
    oled->fillrect(
      start, top + margin + 2, 
      (uint8_t)(start + status->progress * w / 50), top + margin + 20, 
      col_att, col_att);
    oled->fillrect(
      (uint8_t)(start + status->progress * w / 50), top + margin + 2, 
      start + w, top + margin + 20, 
      col_cur, col_cur);
  } else {
    oled->fillrect(
      start, top + margin + 2, 
      start + w, top + margin + 20, 
      col_cur, col_cur);
  }

  // draw button status
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

  // draw debug message
  if (this->debugMessage[0] != '\0') {
    oled->foreground(0xffff);
    oled->locate(0, 48);
    oled->printf("%s", this->debugMessage);  
  }
  if (!fullUpdate) return;
  
  // draw time
  oled->locate(0, 0);
  uint32_t seconds = status->clock / 5;
  uint8_t minute = seconds / 60;
  uint8_t sec = seconds % 60;

  if (status->clock % 5 == 0) {
    tick = !tick;
  }

  oled->foreground(0xffff);
  oled->locate((uint8_t)(6*5.5), 0);
  if (tick) {
    oled->printf("%02d:%02d", minute, sec);
  } else {
    oled->printf("%02d %02d", minute, sec);
  }

  // draw tag counts
  oled->locate(0, 8);
  oled->foreground(COLOR_RED);
  oled->printf("%02d", status->teams[GameStatus::TeamR].tags);

  oled->locate(width - 6*2, 8);
  oled->foreground(COLOR_YEL);
  oled->printf("%02d", status->teams[GameStatus::TeamY].tags);

  // draw points
  oled->locate(0, 0);
  oled->foreground(COLOR_RED);
  oled->printf("%04d", status->teams[GameStatus::TeamR].totalPoint/5);

  oled->locate(width - 6*4, 0);
  oled->foreground(COLOR_YEL);
  oled->printf("%04d", status->teams[GameStatus::TeamY].totalPoint/5);
}

void DisplayController::setGameStatus(GameStatus* status) {
  this->status = status;
}

void DisplayController::debug(const char* msg) {
  uint8_t len = strlen(msg);
  len = len > 15 ? 15 : len;
  memcpy(this->debugMessage, msg, len);
  this->debugMessage[len] = '\0';
}

void DisplayController::debugHex(const char* msg, uint8_t l) {
  uint8_t len = l;
  len = len > 7 ? 7 : len;
  char* p = this->debugMessage;
  for (uint8_t i = 0; i < len; ++i) {
    sprintf(p, "%02X", msg[i]);
    p += 2;
  }
  *p = '\0';
}
