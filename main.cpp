/*

Copyright (c) 2012-2014 RedBearLab

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software
and associated documentation files (the "Software"), to deal in the Software
without restriction,
including without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so,
subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
BE LIABLE
FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

/*
 *    The application works with the BLEController iOS/Android App.
 *    Type something from the Terminal to send
 *    to the BLEController App or vice verse.
 *    Characteristics received from App will print on Terminal.
 */
#define DEVICE_SERIAL_FC 1

#include "mbed.h"
#include "ble/BLE.h"
#include "DFUService.h"
#include "spioled96x64.h"
#include "mcp23s08.h"
#include <string.h>
#include <algorithm>
#include "neopixel.h"
#include "TagDetector.h"

BLE ble;

static const uint8_t DEVICE_NAME[] = "dominator-2";

Serial pc(USBTX, USBRX);

SPIOLED96x64* oled;
mcp23s08* ioxp;
DigitalOut rst(P0_15);
InterruptIn ioInt(P0_28);
volatile bool interrupted = false;
bool tick = false;
neopixel_strip_t strip;
uint8_t neoPixelPin = P0_10;
uint8_t ledsPerStrip = 12;
uint8_t red = 0;
uint8_t green = 0;
uint8_t blue = 0;
uint32_t n = 0;
int8_t dir = 1;

TagDetector tagDetector;

volatile uint8_t buttonR = 1;
volatile uint8_t buttonY = 1;
void updateButtonState () {
  uint8_t val = ioxp->readGpioPort();
  uint8_t bit1 = (val >> 2) & 0x01;
  uint8_t bit2 = (val >> 3) & 0x01;
  buttonY = bit1 ? 0 : 1;
  buttonR = bit2 ? 0 : 1;
}


void disconnectionCallback(const Gap::DisconnectionCallbackParams_t* params) {
}

void initNeoPixel () {
  DigitalOut neoPixelOut(P0_10);
  // neoPixelOut = 1;
  neopixel_init(&strip, neoPixelPin, ledsPerStrip);
  neopixel_clear(&strip);
  neopixel_set_color_and_show(&strip, 0, 80, 10, 10);
}

void radioNotificationCallback (bool radioEnabled) {
  if (!radioEnabled) {
    switch (tagDetector.getState()) {
      case TagDetector::TEAM_RED: 
        red = 255;
        green = 30;
        blue = 30;
        break;
      case TagDetector::TEAM_YELLOW: 
        red = 225;
        green = 225;
        blue = 30;
        break;
      default: 
        red = 90;
        green = 90;
        blue = 80;
        break;
    }
    for (uint8_t p = 0; p < ledsPerStrip; ++p) {
      neopixel_set_color(&strip, p, red - n*3, green - n*3, blue - n*3);
    }
    __disable_irq();
    neopixel_show(&strip);
    __enable_irq();
    n += dir;
    if (dir > 0 && n >= 20) {
      dir = -1;
    } else if (dir < 0 && n == 0) {
      dir = +1;
    }
  }
}



void uartCB(void) {}


uint32_t tickCount = 0;
void tickerCallback() {

  if (tick) {
    ioxp->gpioPort(0b11000000);
  } else {
    ioxp->gpioPort(0b10000000);
  }
  tagDetector.tick();
  updateButtonState();

  oled->locate(90, 56);
  if (buttonR) {
    oled->ChangeFontColor(0b1111100000000000);
    oled->printf("*");
  } else {
    oled->printf(" ");
  }
  oled->locate(0, 56);
  if (buttonY) {
    oled->ChangeFontColor(0b1111111111100000);
    oled->printf("*");
  } else {
    oled->printf(" ");
  }
  oled->ChangeFontColor(0xffff);
  oled->locate(0, 0);
  uint32_t seconds = tickCount / 5;
  uint8_t minute = seconds / 60;
  uint8_t sec = seconds % 60;
  if (tickCount%5 == 0) {
    tick = !tick;
  }
  if (tick) {
    oled->printf("%02d:%02d ", minute, sec);
  } else {
    oled->printf("%02d %02d ", minute, sec);
  }

  oled->ChangeFontColor(0b1111100000000000);
  oled->printf("R:%02d ", tagDetector.getCount(TagDetector::TEAM_RED));

  oled->ChangeFontColor(0b1111111111100000);
  oled->printf("Y:%02d", tagDetector.getCount(TagDetector::TEAM_YELLOW));

  tickCount++;
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

  ioxp->enableInterrupt(4, true);
  ioxp->readGpioPort(); // reset interrupt
  ioInt.mode(PullNone);
  ioInt.fall(&ioxpIntFall);
  ioInt.rise(&ioxpIntRise);
  
  ioxp->gpioPort(0b01000000);
  wait_ms(200);
  ioxp->gpioPort(0b00000000);
  wait_ms(200);
  ioxp->gpioPort(0b01000000);
}
void reset() {
  rst = 1;
  wait_ms(100);
  rst = 0;
  wait_ms(100);
  rst = 1;
  wait_ms(100);
}
void initOled() {
  oled = new SPIOLED96x64(P0_7, P0_15, P0_29, P0_4, P0_8, P0_5);
}

int main(void) {
  NRF_UART0->PSELRTS = 0xFFFFFFFFUL;
  NRF_UART0->PSELCTS = 0xFFFFFFFFUL;

  const uint32_t reason = NRF_POWER->RESETREAS;
  NRF_POWER->RESETREAS = 0xffffffff;  // clear reason
                                      // reset cause should be shown everytime
  pc.baud(9600);
  pc.printf("init [%x] sp:%x\r\n", reason, GET_SP());
  reset();
  initOled();
  initIoxp();
  initNeoPixel();
  ble.init();
  ble.onDisconnection(disconnectionCallback);

  pc.printf("Discover Init \r\n");

  Ticker ticker;
  ticker.attach(tickerCallback, 0.2);

  ble.setDeviceName(DEVICE_NAME);

  /* Enable over-the-air firmware updates. Instantiating DFUSservice introduces
   * a
   * control characteristic which can be used to trigger the application to
   * handover control to a resident bootloader. */
   /*
  DFUService dfu(ble, onStartDfu);

  pc.printf("start advertising \r\n");
  ble.accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED);
  ble.setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
  ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME,
                                   (const uint8_t*)"dom1", sizeof("dom1") - 1);
  ble.setAdvertisingInterval(1000);
  ble.startAdvertising();
  */
  ble.setScanInterval(33);
  ble.setScanWindow(16);
  
  ble.gap().initRadioNotification();
  ble.onRadioNotification(radioNotificationCallback);
  
  // ble.setActiveScan(true);
  pc.printf("Discover Start \r\n");
  ble.gap().startScan(&tagDetector, &TagDetector::scanCallback);
  pc.printf("Discover called \r\n");
  while (1) {
    ble.waitForEvent();
    if (interrupted) { 
      // ioInt.disable_irq();
      interrupted = false;
      // updateButtonState();
      // ioInt.enable_irq();
    }
  }
}