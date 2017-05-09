#include "mbed.h"
#include "ble/BLE.h"
// #include "DFUService.h"
#include "nrf51.h"
#include "mcp23s08.h"
#include <string.h>
#include <algorithm>
#include "neopixel.h"
#include "TagDetector.h"
#include "IM920.h"
#include "DisplayController.h"
#include "GameStatus.h"

BLE ble;
static const uint8_t DEVICE_NAME[] = "dominator-2";

GameStatus status = {0};

//Serial pc(USBTX, USBRX);
IM920 im920(USBTX, USBRX, NC, NC);
char reqBuff[9] = {0x00};
char resBuff[9] = {0x00};
Timeout timeout;

DisplayController* display;

mcp23s08* ioxp;
DigitalOut rst(P0_15);
InterruptIn ioInt(P0_28);
volatile bool interrupted = false;
neopixel_strip_t strip;
uint8_t neoPixelPin = P0_10;
uint8_t ledsPerStrip = 12;
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;
uint32_t n = 0;
int8_t dir = 1;
volatile uint8_t received = 0;

TagDetector tagDetector;

void updateButtonState () {
  uint8_t val = ioxp->readGpioPort();
  uint8_t bit1 = (val >> 2) & 0x01;
  uint8_t bit2 = (val >> 3) & 0x01;
  status.buttons[GameStatus::TeamY] = bit1 ? 0 : 1;
  status.buttons[GameStatus::TeamR] = bit2 ? 0 : 1;
}

void initNeoPixel () {
  DigitalOut neoPixelOut(P0_10);
  // neoPixelOut = 1;
  neopixel_init(&strip, neoPixelPin, ledsPerStrip);
  neopixel_clear(&strip);
  neopixel_set_color_and_show(&strip, 0, 80, 10, 10);
}

inline uint8_t clamp(uint16_t val, uint8_t max) {
  if (val > max) return max;
  return (uint8_t)val;
}

void radioNotificationCallback (bool radioEnabled) {
  if (!radioEnabled) {
    switch (tagDetector.getState()) {
      case TagDetector::TEAM_RED: 
        red = 235;
        green = 10;
        blue = 10;
        break;
      case TagDetector::TEAM_YELLOW: 
        red = 215;
        green = 215;
        blue = 10;
        break;
      default: 
        red = 80;
        green = 80;
        blue = 70;
        break;
    }
    uint16_t mod = 0;
    for (uint8_t r = 0; r < 3; ++r) {
      for (uint8_t h = 0; h < 4; ++h) {
        uint8_t i = (r == 1)? h : 3-h;
        if (h == n) {
          mod = 20;
        } else {
          mod = 0;
        }
        i = i + r*4;
        neopixel_set_color(&strip, i, clamp(red+mod, 255), clamp(green+mod, 255), clamp(blue+mod, 255));
      }
    }
    //__disable_irq();
    neopixel_show(&strip);
    //__enable_irq();
    n++;
    n%=4;
  }
}

void imSendStatus () {
  timeout.detach();
  resBuff[0] = tagDetector.getState();
  resBuff[1] = tagDetector.getCount(0);
  resBuff[2] = tagDetector.getCount(1);
  im920.sendData(resBuff, 8, 0);
}

void uartCB(void) {
  int re = im920.recv(reqBuff, 8);
  if (reqBuff[0] == 0xCF) { // config
    status.cfgSense = -1 * reqBuff[1];
    tagDetector.setSenseLevel(status.cfgSense);
  }
//  timeout.attach(imSendStatus, 0.05 * (rand() % 10));
}

void tickerCallback() {
  tagDetector.tick();
  updateButtonState();
  status.state = tagDetector.getState();
  status.teams[GameStatus::TeamR].detectedTags = tagDetector.getCount(GameStatus::TeamR);
  status.teams[GameStatus::TeamY].detectedTags = tagDetector.getCount(GameStatus::TeamY);
  status.clock++;
  display->update();

  if (status.clock % 5 == 0) {
    timeout.attach(imSendStatus, 0.05 * (rand() % 10));
  }
}

void onStartDfu() {
  ble.stopScan();
}

void ioxpIntRise() {
  interrupted = true;
}
void ioxpIntFall() {
  interrupted = true;
}
void initIoxp() {
  ioxp = new mcp23s08(P0_4, P0_8, P0_5, P0_6, 0x20);

  ioxp->begin();
  ioxp->gpioPinMode(0b00001100);
  ioxp->portPullup(0b00001100);

  ioxp->enableInterrupt(2, true);
  ioxp->enableInterrupt(3, true);
  ioxp->readGpioPort(); // reset interrupt
  ioInt.mode(PullNone);
  ioInt.fall(&ioxpIntFall);
  ioInt.rise(&ioxpIntRise);

  ioxp->gpioPort(0b11110000);
  wait_ms(100);
  ioxp->gpioPort(0b11000000);
  wait_ms(100);
  ioxp->gpioPort(0b11110000);
  wait_ms(100);
  ioxp->gpioPort(0b11000000);
}

void reset() {
  rst = 1;
  wait_ms(100);
  rst = 0;
  wait_ms(100);
  rst = 1;
  wait_ms(100);
}

int main(void) {
  NRF_UART0->PSELRTS = 0xFFFFFFFFUL;
  NRF_UART0->PSELCTS = 0xFFFFFFFFUL;

  //const uint32_t reason = NRF_POWER->RESETREAS;
  NRF_POWER->RESETREAS = 0xffffffff;  // clear reason
                                      // reset cause should be shown everytime
  reset();
  im920.init();
  im920.attach(uartCB);
  status.nodeId = im920.getNode();
  display = new DisplayController(P0_7, P0_15, P0_29, P0_4, P0_8, P0_5);
  display->setGameStatus(&status);
  initIoxp();
  initNeoPixel();
  ble.init();

  Ticker ticker;
  NVIC_SetPriority(UART0_IRQn, 1);
  NVIC_SetPriority(TIMER1_IRQn, 2);
  ticker.attach(tickerCallback, 0.2);

  ble.setDeviceName(DEVICE_NAME);

   /*
  DFUService dfu(ble, onStartDfu);
  */
  ble.setScanInterval(33);
  ble.setScanWindow(16);
  
  ble.gap().initRadioNotification();
  ble.onRadioNotification(radioNotificationCallback);
  
  // ble.gap().setActiveScan(true);
  ble.gap().startScan(&tagDetector, &TagDetector::scanCallback);
  while (1) {
    im920.poll();
    ble.waitForEvent();
  }
}