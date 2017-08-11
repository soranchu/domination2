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
#include "commands.h"

BLE ble;
PinName PIN_RST = P0_15;
PinName PIN_MOSI = P0_4;
PinName PIN_MISO = P0_8;
PinName PIN_SCLK = P0_5;
PinName PIN_IO_INT = P0_28;
PinName PIN_LED = P0_10;
PinName PIN_SMLED = P0_19;
PinName PIN_IO_CS = P0_6;
PinName PIN_OLED_CS = P0_7;
PinName PIN_OLED_DC = P0_29;
DigitalIn dummyMiso(PIN_MISO, PullUp);
DigitalIn dummyMosi(PIN_MOSI, PullUp);

static const uint8_t DEVICE_NAME[] = "dominator-2";

GameStatus status = {0};

//Serial pc(USBTX, USBRX);
IM920 im920(USBTX, USBRX, NC, NC);
char reqBuff[12] = {0x00};
char resBuff[12] = {0x00};
Timeout timeout;

DisplayController* display;

mcp23s08* ioxp;
DigitalOut rst(PIN_RST);
DigitalOut smled(PIN_SMLED, PullUp);
InterruptIn ioInt(PIN_IO_INT);
volatile bool interrupted = false;
neopixel_strip_t strip;
uint8_t ledsPerStrip = 12;
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;
uint32_t n = 0;
int8_t dir = 1;
uint8_t lastStatus = 0xff;
volatile uint8_t received = 0;
uint32_t tick = 0;

TagDetector tagDetector;

void beep (uint8_t vol) {
  if (vol) {
    ioxp->gpioPort(0b11110000);
  } else {
    ioxp->gpioPort(0b11100000);
  }
  wait_ms((vol+1) * 30);
  ioxp->gpioPort(0b11000000);
}

void updateButtonState () {
  uint8_t val = ioxp->readGpioPort();
  if ((val & 0xc0) == 0xc0) {
    uint8_t bitR = (val >> 3) & 0x01;
    uint8_t bitY = (val >> 2) & 0x01;
    status.teams[GameStatus::TeamY].button = bitY ? 0 : 1;
    status.teams[GameStatus::TeamR].button = bitR ? 0 : 1;
    resBuff[3] = val;
  }
}

void initNeoPixel () {
  DigitalOut neoPixelOut(PIN_LED);
  // neoPixelOut = 1;
  neopixel_init(&strip, PIN_LED, ledsPerStrip);
  neopixel_clear(&strip);
  neopixel_set_color_and_show(&strip, 0, 80, 10, 10);
}

inline uint8_t clamp(int16_t val, uint8_t max) {
  if (val < 0) return 0;
  if (val > max) return max;
  return (uint8_t)val;
}

void radioNotificationCallback (bool radioEnabled) {
  if (!radioEnabled) {
    updateButtonState();
    uint8_t s = status.current;

    if (lastStatus != s) {
      if (s != 0xff) {
        beep(1);
      } else {
        beep(0);
      }
      lastStatus = s;
    }
    switch (s) {
      case GameStatus::TeamR: 
        red = 205;
        green = 15;
        blue = 15;
        break;
      case GameStatus::TeamY: 
        red = 165;
        green = 165;
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
        /*
        if (h == n) {
          mod = 20;
        } else {
          mod = 0;
        }*/
        if (n < 20) {
          mod = n - 20;
        } else if ( n < 40) {
          mod = 0;
        } else {
          mod = 40 - n;
        }
        mod*=2;
        i = i + r*4;
        neopixel_set_color(&strip, i, clamp(red+mod, 255), clamp(green+mod, 255), clamp(blue+mod, 255));
      }
    }
    __disable_irq();
    neopixel_show(&strip);
    __enable_irq();
    n++;
    n%=60;
  }
}

void imSendStatus () {
  timeout.detach();
  resBuff[0] = status.current;
  resBuff[1] = status.teams[GameStatus::TeamR].tags;
  resBuff[2] = status.teams[GameStatus::TeamY].tags;
  uint16_t* p16 = (uint16_t*)(resBuff+3); // [3][4]
  *p16 = status.teams[GameStatus::TeamR].point;
  p16 = (uint16_t*)(resBuff+5); // [5][6]
  *p16 = status.teams[GameStatus::TeamY].point;
  resBuff[7] = status.started;
  //uint32_t* p32 = (uint32_t*)(resBuff+8); // [8][9][10][11]
  //*p32 = status.clock;
  im920.sendData(resBuff, 8, 0);
//  display->debugHex(resBuff, 12);
}

void uartCB(void) {
  im920.recv(reqBuff, 10);
  uint16_t* p;
  uint32_t* c;
  switch (reqBuff[0]) {
  case CMD_STATUS:
    status.started = reqBuff[9] == 0x01;
    p = (uint16_t*)(reqBuff+5);
    status.teams[GameStatus::TeamR].totalPoint = *p;
    p = (uint16_t*)(reqBuff+7);
    status.teams[GameStatus::TeamY].totalPoint = *p;
    c = (uint32_t*)(reqBuff+1);
    status.clock = *c / 1000 * 5;
    if (reqBuff[9] == 0x09) {
      status.reset();
      tagDetector.reset();
    }
    break;
  case CMD_START:
    status.started = true;
    break;
  case CMD_PAUSE:
    status.started = false;
    break;
  case CMD_STOP:
    status.started = false;
    break;
  case CMD_CFG:
    status.cfgSense = -1 * reqBuff[1];
    tagDetector.setSenseLevel(status.cfgSense);
    break;
  case CMD_RESET:
    status.reset();
    tagDetector.reset();
    break;
  }
  timeout.attach(imSendStatus, 0.1 + 0.06 * (rand() % 10));
}

void calcGameStatus () {
  uint8_t r = status.teams[GameStatus::TeamR].tags + status.teams[GameStatus::TeamR].button;
  uint8_t y = status.teams[GameStatus::TeamY].tags + status.teams[GameStatus::TeamY].button;
  uint8_t diff = 0;

  if (status.current != 0xff) {
    status.teams[status.current].point++;
  }

  if (r == y) {
    status.decreaseProgress();
    return;
  }
  
  uint8_t attacker;
  if (r > y) {
    attacker = GameStatus::TeamR;
    diff = r - y;
  } else {
    attacker = GameStatus::TeamY;
    diff = y - r;
  }

  if (attacker == status.current) {
    status.decreaseProgress();
    return;
  }

  if (attacker != status.attacker) {
    status.resetProgress();
    status.attacker = attacker;
  }

  status.increaseProgress(diff);
  if (status.progress > 50) {
    status.current = status.attacker;
    status.resetProgress();
  }
}
void tickerCallback () {
  bool fullUpdate = false;
  tick++;
  if (status.started) {
    tagDetector.tick();
    if (status.teams[GameStatus::TeamR].tags != tagDetector.getCount(GameStatus::TeamR) ||
        status.teams[GameStatus::TeamY].tags != tagDetector.getCount(GameStatus::TeamY)) {
      beep(1);
      fullUpdate = true;
    }
    status.teams[GameStatus::TeamR].tags = tagDetector.getCount(GameStatus::TeamR);
    status.teams[GameStatus::TeamY].tags = tagDetector.getCount(GameStatus::TeamY);
    if (status.clock > 0) {
      status.clock--;
    }
    calcGameStatus();
  }
  if (tick % 5 == 0) {
    smled = 0;
    display->update(true);
    smled = 1;
  } else {
//    updateButtonState();
    display->update(fullUpdate);
  }
  if (tick % 5 == 0) {
    //timeout.attach(imSendStatus, 0.05 * (rand() % 10));
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
  ioxp = new mcp23s08(PIN_MOSI, PIN_MISO, PIN_SCLK, PIN_IO_CS, 0x20);

  ioxp->begin();
  ioxp->gpioPinMode(0b00001100);
  ioxp->portPullup(0b00001100);

  ioxp->enableInterrupt(2, true);
  ioxp->enableInterrupt(3, true);
  ioxp->readGpioPort(); // reset interrupt
  ioInt.mode(PullNone);
  ioInt.fall(&ioxpIntFall);
  ioInt.rise(&ioxpIntRise);

  beep(1);
  wait_ms(100);
  beep(1);
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
  status.reset();

  reset();
  im920.init();
  im920.attach(uartCB);
  status.nodeId = im920.getNode();
  display = new DisplayController(PIN_OLED_CS, PIN_RST, PIN_OLED_DC, PIN_MOSI, PIN_MISO, PIN_SCLK);
  initIoxp();
  display->setGameStatus(&status);
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