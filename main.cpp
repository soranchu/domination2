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
#include <set>
#include <algorithm>
#include "neopixel.h"

BLE ble;

const uint8_t DEVICE_NAME[] = "dominator-2";

Serial pc(USBTX, USBRX);

SPIOLED96x64* oled;
mcp23s08* ioxp;
DigitalOut rst(P0_15);
InterruptIn ioInt(P0_28);
volatile bool interrupted = false;
bool tick = false;
neopixel_strip_t strip;
uint8_t neoPixelPin = P0_10;
uint8_t ledsPerStrip = 15;
uint8_t red = 255;
uint8_t green = 0;
uint8_t blue = 159;
uint32_t n = 0;
struct Tag_t {
  const BLEProtocol::AddressBytes_t address; /**< The BLE address. */
  const char type;
};
const uint8_t TAG_SIZE = 30;
Tag_t const TAGS[] = {
    {{0xfc, 0x58, 0xfa, 0x19, 0xdf, 0x7f}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xbc, 0x81}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xbe, 0x3b}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xbe, 0x18}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xc6, 0x22}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xc0, 0x8b}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xbc, 0xc8}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xcc, 0x27}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xbd, 0xc8}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xbe, 0x34}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xc5, 0xef}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xce, 0xb1}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xdf, 0x78}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xc6, 0x09}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xc2, 0x84}, 'R'},  // R
    {{0xfc, 0x58, 0xfa, 0x19, 0xe6, 0xfb}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe7, 0x93}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0x77, 0x4f}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xdf, 0xc5}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe5, 0x99}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe8, 0x34}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe7, 0xcb}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe5, 0x4a}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe4, 0x8c}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe6, 0xdb}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0x77, 0x82}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0x68, 0x8a}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe6, 0xcf}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe5, 0x47}, 'Y'},  // Y
    {{0xfc, 0x58, 0xfa, 0x19, 0xe5, 0x77}, 'Y'}   // Y
};
struct Team_t {
  uint8_t tagCount;
  char type;
};
Team_t teams[] = {{0, 'R'}, {0, 'Y'}};
int8_t nodeState = -1;
const uint8_t TEAM_RED = 0;
const uint8_t TEAM_YELLOW = 1;

struct TagStatus_t {
  volatile bool visible;
  volatile uint8_t visibleCount;
};

TagStatus_t statuses[30] = {0};

struct Peripheral_t {
  BLEProtocol::AddressBytes_t address; /**< The BLE address. */

  /**
   * Construct an Address_t object with the supplied type and address.
   *
   * @param[in] typeIn
   *              The BLE address type.
   * @param[in] addressIn
   *              The BLE address.
   */
  Peripheral_t(const BLEProtocol::AddressBytes_t addressIn) {
    std::copy(addressIn, addressIn + BLEProtocol::ADDR_LEN, address);
  }

  /**
   * Empty constructor.
   */
  Peripheral_t() : address() {}
};

bool tagIdxToTeam(uint8_t idx) {
  return idx < 15 ? TEAM_RED : TEAM_YELLOW;
}

bool operator<(const Peripheral_t& t1, const Peripheral_t& t2) {
  return memcmp(t1.address, t2.address, BLEProtocol::ADDR_LEN) < 0;
}
bool operator>(const Peripheral_t& t1, const Peripheral_t& t2) {
  return t2 < t1;
}
bool operator<=(const Peripheral_t& t1, const Peripheral_t& t2) {
  return !(t1 > t2);
}
bool operator>=(const Peripheral_t& t1, const Peripheral_t& t2) {
  return !(t1 < t2);
}
bool operator==(const Peripheral_t& t1, const Peripheral_t& t2) {
  return !(t1 < t2) && !(t2 > t2);
}  // ...(*1)
bool operator!=(const Peripheral_t& t1, const Peripheral_t& t2) {
  return !(t1 == t2);
}

std::set<Peripheral_t> devices;
static const uint8_t SERVICES[6] = {0x03, 0x18, 0x02, 0x18, 0x04, 0x18};

int8_t findTagId(const BLEProtocol::AddressBytes_t address) {
  for (uint8_t i = 0; i < TAG_SIZE; ++i) {
    if (memcmp(address, TAGS[i].address, 6) == 0) {
      return i;
    }
  }
  return -1;
}

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
  neopixel_set_color_and_show(&strip, 0, 255, 255, 255);
}

void radioNotificationCallback (bool radioEnabled) {
  if (!radioEnabled) {
    switch (nodeState) {
      case TEAM_RED: 
        red = 255;
        green = 10;
        blue = 10;
        break;
      case TEAM_YELLOW: 
        red = 255;
        green = 255;
        blue = 10;
        break;
      default: 
        red = 90;
        green = 90;
        blue = 80;
        break;
    }
    for (uint8_t p = 0; p < ledsPerStrip; ++p) {
        if (n%ledsPerStrip == p) {
          neopixel_set_color(&strip, p, red, green, blue);
        } else {
          neopixel_set_color(&strip, p, red - 10, green - 10, blue - 10);
        }
    }
    __disable_irq();
    neopixel_show(&strip);
    __enable_irq();
    n++;
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
  teams[TEAM_RED].tagCount = 0;
  teams[TEAM_YELLOW].tagCount = 0;

  for (uint8_t i = 0; i < TAG_SIZE; ++i) {
    TagStatus_t& s = statuses[i];
    if (s.visibleCount > 0) {
      s.visibleCount--;
    }
    if (s.visibleCount > 0) {
      teams[tagIdxToTeam(i)].tagCount++;
    }
  }
  if (teams[TEAM_RED].tagCount == teams[TEAM_YELLOW].tagCount) {
    nodeState = -1;
  } else if (teams[TEAM_RED].tagCount > teams[TEAM_YELLOW].tagCount) {
    nodeState = 0;
  } else {
    nodeState = 1;
  }

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
  oled->printf("R:%02d ", teams[TEAM_RED].tagCount);

  oled->ChangeFontColor(0b1111111111100000);
  oled->printf("Y:%02d", teams[TEAM_YELLOW].tagCount);

  tickCount++;
}

void scanCallback(const Gap::AdvertisementCallbackParams_t* params) {
  // parse the advertising payload, looking for data type COMPLETE_LOCAL_NAME
  // The advertising payload is a collection of key/value records where
  // byte 0: length of the record excluding this byte
  // byte 1: The key, it is the type of the data
  // byte [2..N] The value. N is equal to byte0 - 1

  for (uint8_t i = 0; i < params->advertisingDataLen; ++i) {
    const uint8_t record_length = params->advertisingData[i];
    if (record_length == 0) {
      continue;
    }
    const uint8_t type = params->advertisingData[i + 1];
    const uint8_t* value = params->advertisingData + i + 2;
    const uint8_t value_length = record_length - 1;

    i += record_length;
    if (value_length == 6 && memcmp(value, SERVICES, 6) == 0) {
      Peripheral_t peri(params->peerAddr);
      std::pair<std::set<Peripheral_t>::iterator, bool> ret =
          devices.insert(peri);
      if (ret.second) {
      }
      int8_t idx = findTagId(ret.first->address);
      if (idx >= 0) {
        statuses[idx].visibleCount = 10;
      }
      break;
    }
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
  ble.startScan(scanCallback);
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