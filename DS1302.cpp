#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "DS1302.h"

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

Time::Time() {
  Time(2000, 1, 1, 0, 0, 0, kSaturday);
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

uint8_t DS1302::registerBcdToDec(const Register reg, const uint8_t high_bit) {
  const uint8_t mask = (1 << (high_bit + 1)) - 1;
  uint8_t val = readRegister(reg);
  val &= mask;
  val = (val & 15) + 10 * ((val & (15 << 4)) >> 4);
  return val;
}

uint8_t DS1302::registerBcdToDec(const Register reg) {
  return registerBcdToDec(reg, 7);
}

void DS1302::registerDecToBcd(const Register reg, uint8_t value,
                              const uint8_t high_bit) {
  const uint8_t mask = (1 << (high_bit + 1)) - 1;
  uint8_t regv = readRegister(reg);

  // Convert value to bcd in place.
  uint8_t tvalue = value / 10;
  value = value % 10;
  value |= (tvalue << 4);

  // Replace high bits of value if needed.
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
  return registerBcdToDec(kSecondReg, 6);
}

uint8_t DS1302::minutes() {
  return registerBcdToDec(kMinuteReg);
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
  return registerBcdToDec(kDateReg, 5);
}

uint8_t DS1302::month() {
  return registerBcdToDec(kMonthReg, 4);
}

Time::Day DS1302::day() {
  return static_cast<Time::Day>(registerBcdToDec(kDayReg, 2));
}

uint16_t DS1302::year() {
  return 2000 + registerBcdToDec(kYearReg);
}

Time DS1302::time() {
  Time t;
  t.sec  = seconds();
  t.min  = minutes();
  t.hr   = hour();
  t.date = date();
  t.mon  = month();
  t.day  = day();
  t.yr   = year();
  return t;
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
