
#include "mbed.h"
#include "mcp23s08.h"

#define INPUT 0
#define OUTPUT 1

#define LOW 0
#define HIGH 1

mcp23s08::mcp23s08(PinName mosi,
                   PinName miso,
                   PinName clk,
                   PinName cs_pin,
                   const uint8_t haenAdrs)
    : SPI(mosi, miso, clk), cs(cs_pin) {
  
  _gpioDirection = 0xFF;
  _gpioState = 0x00;
  _gpIntEnable = 0x00;
  _defVal = 0x00;
  _intControl = 0x00;

  format(8, 3);
  frequency(2 * 1000 * 1000); //2.4MHz

  postSetup(haenAdrs);
}

void mcp23s08::postSetup(const uint8_t haenAdrs) {
  if (haenAdrs >= 0x20 && haenAdrs <= 0x23) {  // HAEN works between 0x20...0x23
    _adrs = haenAdrs;
    _useHaen = 1;
  } else {
    _adrs = 20;
    _useHaen = 0;
  }
  _readCmd = (_adrs << 1) | 1;
  _writeCmd = _adrs << 1;
  // setup register values for this chip
  IOCON = 0x05;
  IODIR = 0x00;
  GPPU = 0x06;
  GPIO = 0x09;
  GPINTEN = 0x02;
  IPOL = 0x01;
  DEFVAL = 0x03;
  INTF = 0x07;
  INTCAP = 0x08;
  OLAT = 0x0A;
  INTCON = 0x04;
}

void mcp23s08::begin(bool protocolInitOverride) {
  cs = 1;
  wait(0.1);
  _useHaen == 1 ? writeByte(IOCON, 0b00111000) : writeByte(IOCON, 0b00110000);
}

void mcp23s08::gpioPinMode(uint8_t mode) {
  _gpioDirection = mode;
  writeByte(IODIR, _gpioDirection);
}

void mcp23s08::gpioPinMode(uint8_t pin, bool mode) {
  if (pin < 8) {  // 0...7
    mode == INPUT ? _gpioDirection |= (1 << pin)
                  : _gpioDirection &= ~(1 << pin);
    writeByte(IODIR, _gpioDirection);
  }
}

void mcp23s08::gpioPort(uint8_t value) {
  _gpioState = value;
  writeByte(GPIO, _gpioState);
}

uint8_t mcp23s08::readGpioPort() {
  return gpioRegisterReadByte(GPIO);
}

uint8_t mcp23s08::readGpioPortFast() {
  return _gpioState;
}

uint8_t mcp23s08::gpioDigitalReadFast(uint8_t pin) {
  if (pin < 8) {  // 0...7
    int temp = _gpioState & (1 << pin);
    return temp;
  } else {
    return 0;
  }
}

void mcp23s08::portPullup(uint8_t data) {
  writeByte(GPPU, data);
}

void mcp23s08::gpioDigitalWrite(uint8_t pin, bool value) {
  if (pin < 8) {  // 0...7
    value == HIGH ? _gpioState |= (1 << pin) : _gpioState &= ~(1 << pin);
    writeByte(GPIO, _gpioState);
  }
}

void mcp23s08::gpioDigitalWriteFast(uint8_t pin, bool value) {
  if (pin < 8) {  // 0...8
    value == HIGH ? _gpioState |= (1 << pin) : _gpioState &= ~(1 << pin);
  }
}

void mcp23s08::gpioPortUpdate() {
  writeByte(GPIO, _gpioState);
}

uint8_t mcp23s08::gpioDigitalRead(uint8_t pin) {
  if (pin < 8)
    return (int)(gpioRegisterReadByte(GPIO) & 1 << pin);
  return 0;
}

uint8_t mcp23s08::gpioRegisterReadByte(uint8_t reg) {
  uint8_t data = 0;
  startSend(1);
  SPI::write(reg);
  data = (uint8_t)SPI::write(0x00);
  endSend();
  return data;
}

void mcp23s08::gpioRegisterWriteByte(uint8_t reg, uint8_t data) {
  writeByte(reg, (uint8_t)data);
}

void mcp23s08::enableInterrupt(uint8_t pin, bool enable, uint8_t defVal) {
  if (pin > 7)
    return;

  if (enable) {
    _gpIntEnable |= (1 << pin);
  } else {
    _gpIntEnable &= ~(1 << pin);
  }
  writeByte(GPINTEN, _gpIntEnable);

  if (!enable) return;

  if (defVal == 0x01) {
    _defVal |= (1 << pin);
  } else {
    _defVal &= ~(1 << pin);
  }
  writeByte(DEFVAL, _defVal);

  _intControl |= (1 << pin); 
  writeByte(INTCON, _intControl);
}
void mcp23s08::enableInterrupt(uint8_t pin, bool enable) {
  if (pin > 7)
    return;

  if (enable) {
    _gpIntEnable |= (1 << pin);
  } else {
    _gpIntEnable &= ~(1 << pin);
  }
  writeByte(GPINTEN, _gpIntEnable);

  if (!enable) return;
  
  _intControl &= ~(1 << pin); 
  writeByte(INTCON, _intControl);
}
uint8_t mcp23s08::readInterruptFlag() {
  return gpioRegisterReadByte(INTF);
}
uint8_t mcp23s08::readInterruptCapture() {
  return gpioRegisterReadByte(INTCAP);
}

/* ------------------------------ Low Level ----------------*/
void mcp23s08::startSend(bool mode) {
  cs = 0;
  mode == 1 ? SPI::write(_readCmd) : SPI::write(_writeCmd);
}

void mcp23s08::endSend() {
  cs = 1;
}

void mcp23s08::writeByte(uint8_t addr, uint8_t data) {
  startSend(0);
  SPI::write(addr);
  SPI::write(data);
  endSend();
}
