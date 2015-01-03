#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "DS1302.h"

#include <stdint.h>

namespace {

// The RAM register space follows the clock register space.
const uint8_t kRamRegisterOffset = 32;

enum Register {
  kSecondReg       = 0,
  kMinuteReg       = 1,
  kHourReg         = 2,
  kDateReg         = 3,
  kMonthReg        = 4,
  kDayReg          = 5,
  kYearReg         = 6,
  kWriteProtectReg = 7
};

// Returns the decoded decimal value from a binary-coded decimal (BCD) byte.
// Assumes 'bcd' is coded with 4-bits per digit, with the tens place digit in
// the upper 4 MSBs.
uint8_t bcdToDec(const uint8_t bcd) {
  return (10 * ((bcd & 0xF0) >> 4) + (bcd & 0x0F));
}

// Returns the binary-coded decimal of 'dec'. Inverse of bcdToDec.
uint8_t decToBcd(const uint8_t dec) {
  const uint8_t tens = dec / 10;
  const uint8_t ones = dec % 10;
  return (tens << 4) | ones;
}

uint8_t hourFromRegisterValue(const uint8_t value) {
  uint8_t adj;
  if (value & 128)  // 12-hour mode
    adj = 12 * ((value & 32) >> 5);
  else           // 24-hour mode
    adj = 10 * ((value & (32 + 16)) >> 4);
  return (value & 15) + adj;
}

}  // namespace

Time::Time(const uint16_t yr, const uint8_t mon, const uint8_t date,
           const uint8_t hr, const uint8_t min, const uint8_t sec,
           const Day day) {
  this->yr   = yr;
  this->mon  = mon;
  this->date = date;
  this->hr   = hr;
  this->min  = min;
  this->sec  = sec;
  this->day  = day;
}

DS1302::DS1302(const uint8_t ce_pin, const uint8_t io_pin,
               const uint8_t sclk_pin) {
  ce_pin_ = ce_pin;
  io_pin_ = io_pin;
  sclk_pin_ = sclk_pin;

  pinMode(ce_pin, OUTPUT);
  pinMode(sclk_pin, OUTPUT);
}

void DS1302::writeOut(const uint8_t value) {
  pinMode(io_pin_, OUTPUT);
  shiftOut(io_pin_, sclk_pin_, LSBFIRST, value);
}

uint8_t DS1302::readIn() {
  uint8_t input_value = 0;
  uint8_t bit = 0;
  pinMode(io_pin_, INPUT);

  for (int i = 0; i < 8; ++i) {
    bit = digitalRead(io_pin_);
    input_value |= (bit << i);

    digitalWrite(sclk_pin_, HIGH);
    delayMicroseconds(1);
    digitalWrite(sclk_pin_, LOW);
  }

  return input_value;
}

uint8_t DS1302::readRegister(const uint8_t reg) {
  uint8_t cmd_byte = 129;  // 1000 0001
  uint8_t reg_value;
  cmd_byte |= (reg << 1);

  digitalWrite(sclk_pin_, LOW);
  digitalWrite(ce_pin_, HIGH);

  writeOut(cmd_byte);
  reg_value = readIn();

  digitalWrite(ce_pin_, LOW);

  return reg_value;
}

void DS1302::writeRegister(const uint8_t reg, const uint8_t value) {
  uint8_t cmd_byte = (128 | (reg << 1));

  digitalWrite(sclk_pin_, LOW);
  digitalWrite(ce_pin_, HIGH);

  writeOut(cmd_byte);
  writeOut(value);

  digitalWrite(ce_pin_, LOW);
}

void DS1302::writeProtect(const bool enable) {
  writeRegister(kWriteProtectReg, (enable << 7));
}

void DS1302::halt(const bool enable) {
  uint8_t sec = readRegister(kSecondReg);
  sec &= ~(1 << 7);
  sec |= (enable << 7);
  writeRegister(kSecondReg, sec);
}

Time DS1302::time() {
  Time t(2099, 1, 1, 0, 0, 0, Time::kSunday);

  const uint8_t cmd_byte = 0xBF;  // Clock burst read.
  digitalWrite(sclk_pin_, LOW);
  digitalWrite(ce_pin_, HIGH);
  writeOut(cmd_byte);

  t.sec = bcdToDec(readIn() & 0x7F);
  t.min = bcdToDec(readIn());
  t.hr = hourFromRegisterValue(readIn());
  t.date = bcdToDec(readIn());
  t.mon = bcdToDec(readIn());
  t.day = static_cast<Time::Day>(bcdToDec(readIn()));
  t.yr = 2000 + bcdToDec(readIn());

  digitalWrite(ce_pin_, LOW);

  return t;
}

void DS1302::time(const Time t) {
  // We want to maintain the Clock Halt flag if it is set.
  const uint8_t ch_value = readRegister(kSecondReg) & 0x80;

  const uint8_t cmd_byte = 0xBE;  // Clock burst write.
  digitalWrite(sclk_pin_, LOW);
  digitalWrite(ce_pin_, HIGH);
  writeOut(cmd_byte);

  writeOut(ch_value | decToBcd(t.sec));
  writeOut(decToBcd(t.min));
  writeOut(decToBcd(t.hr));
  writeOut(decToBcd(t.date));
  writeOut(decToBcd(t.mon));
  writeOut(decToBcd(static_cast<uint8_t>(t.day)));
  writeOut(decToBcd(t.yr - 2000));
  writeOut(0);  // Write protection register.

  digitalWrite(ce_pin_, LOW);
}

void DS1302::writeRam(const uint8_t address, const uint8_t value) {
  // Only RAM addresses in [0, 30] are valid.
  if (address > 30) {
    return;
  }

  writeRegister(kRamRegisterOffset + address, value);
}

uint8_t DS1302::readRam(const uint8_t address) {
  // Only RAM addresses in [0, 30] are valid.
  if (address > 30) {
    return 0;
  }

  return readRegister(kRamRegisterOffset + address);
}
