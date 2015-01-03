#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "DS1302.h"

namespace {

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

void DS1302::registerDecToBcd(const Register reg, uint8_t value,
                              const uint8_t high_bit) {
  const uint8_t mask = (1 << (high_bit + 1)) - 1;
  uint8_t regv = readRegister(reg);

  // Convert value to bcd in place.
  value = decToBcd(value);

  // Replace high bits of register if needed.
  value &= mask;
  value |= (regv &= ~mask);

  writeRegister(reg, value);
}

void DS1302::registerDecToBcd(const Register reg, const uint8_t value) {
  registerDecToBcd(reg, value, 7);
}

uint8_t DS1302::readRegister(const Register reg) {
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

void DS1302::writeRegister(const Register reg, const uint8_t value) {
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

uint8_t DS1302::seconds() {
  // Bit 7 is the Clock Halt (CH) flag. Remove it before decoding.
  return bcdToDec(readRegister(kSecondReg) & 0x7F);
}

uint8_t DS1302::minutes() {
  return bcdToDec(readRegister(kMinuteReg));
}

uint8_t DS1302::hour() {
  uint8_t hr = readRegister(kHourReg);
  uint8_t adj;
  if (hr & 128)  // 12-hour mode
    adj = 12 * ((hr & 32) >> 5);
  else           // 24-hour mode
    adj = 10 * ((hr & (32 + 16)) >> 4);
  hr = (hr & 15) + adj;
  return hr;
}

uint8_t DS1302::date() {
  return bcdToDec(readRegister(kDateReg));
}

uint8_t DS1302::month() {
  return bcdToDec(readRegister(kMonthReg));
}

Time::Day DS1302::day() {
  return static_cast<Time::Day>(bcdToDec(readRegister(kDayReg)));
}

uint16_t DS1302::year() {
  return 2000 + bcdToDec(readRegister(kYearReg));
}

Time DS1302::time() {
  return Time(year(), month(), date(),
              hour(), minutes(), seconds(),
              day());
}

void DS1302::seconds(const uint8_t sec) {
  registerDecToBcd(kSecondReg, sec, 6);
}

void DS1302::minutes(const uint8_t min) {
  registerDecToBcd(kMinuteReg, min, 6);
}

void DS1302::hour(const uint8_t hr) {
  writeRegister(kHourReg, 0);  // set 24-hour mode
  registerDecToBcd(kHourReg, hr, 5);
}

void DS1302::date(const uint8_t date) {
  registerDecToBcd(kDateReg, date, 5);
}

void DS1302::month(const uint8_t mon) {
  registerDecToBcd(kMonthReg, mon, 4);
}

void DS1302::day(const Time::Day day) {
  registerDecToBcd(kDayReg, static_cast<int>(day), 2);
}

void DS1302::year(uint16_t yr) {
  yr -= 2000;
  registerDecToBcd(kYearReg, yr);
}

void DS1302::time(const Time t) {
  seconds(t.sec);
  minutes(t.min);
  hour(t.hr);
  date(t.date);
  month(t.mon);
  day(t.day);
  year(t.yr);
}
