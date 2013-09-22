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
  _ce_pin = ce_pin;
  _io_pin = io_pin;
  _sclk_pin = sclk_pin;

  pinMode(ce_pin, OUTPUT);
  pinMode(sclk_pin, OUTPUT);
}

void DS1302::_write_out(const uint8_t value) {
  pinMode(_io_pin, OUTPUT);
  shiftOut(_io_pin, _sclk_pin, LSBFIRST, value);
}

uint8_t DS1302::_read_in() {
  uint8_t input_value = 0;
  uint8_t bit = 0;
  pinMode(_io_pin, INPUT);

  for (int i = 0; i < 8; ++i) {
    bit = digitalRead(_io_pin);
    input_value |= (bit << i);

    digitalWrite(_sclk_pin, HIGH);
    delayMicroseconds(1);
    digitalWrite(_sclk_pin, LOW);
  }

  return input_value;
}

uint8_t DS1302::_register_bcd_to_dec(const reg_t reg, const uint8_t high_bit) {
  const uint8_t mask = (1 << (high_bit + 1)) - 1;
  uint8_t val = read_register(reg);
  val &= mask;
  val = (val & 15) + 10 * ((val & (15 << 4)) >> 4);
  return val;
}

uint8_t DS1302::_register_bcd_to_dec(const reg_t reg) {
  return _register_bcd_to_dec(reg, 7);
}

void DS1302::_register_dec_to_bcd(const reg_t reg, uint8_t value,
                                  const uint8_t high_bit) {
  const uint8_t mask = (1 << (high_bit + 1)) - 1;
  uint8_t regv = read_register(reg);

  // Convert value to bcd in place.
  uint8_t tvalue = value / 10;
  value = value % 10;
  value |= (tvalue << 4);

  // Replace high bits of value if needed.
  value &= mask;
  value |= (regv &= ~mask);

  write_register(reg, value);
}

void DS1302::_register_dec_to_bcd(const reg_t reg, const uint8_t value) {
  _register_dec_to_bcd(reg, value, 7);
}

uint8_t DS1302::read_register(const reg_t reg) {
  uint8_t cmd_byte = 129;  // 1000 0001
  uint8_t reg_value;
  cmd_byte |= (reg << 1);

  digitalWrite(_sclk_pin, LOW);
  digitalWrite(_ce_pin, HIGH);

  _write_out(cmd_byte);
  reg_value = _read_in();

  digitalWrite(_ce_pin, LOW);

  return reg_value;
}

void DS1302::write_register(const reg_t reg, const uint8_t value) {
  uint8_t cmd_byte = (128 | (reg << 1));

  digitalWrite(_sclk_pin, LOW);
  digitalWrite(_ce_pin, HIGH);

  _write_out(cmd_byte);
  _write_out(value);

  digitalWrite(_ce_pin, LOW);
}

void DS1302::write_protect(const bool enable) {
  write_register(WP_REG, (enable << 7));
}

void DS1302::halt(const bool enable) {
  uint8_t sec = read_register(SEC_REG);
  sec &= ~(1 << 7);
  sec |= (enable << 7);
  write_register(SEC_REG, sec);
}

uint8_t DS1302::seconds() {
  return _register_bcd_to_dec(SEC_REG, 6);
}

uint8_t DS1302::minutes() {
  return _register_bcd_to_dec(MIN_REG);
}

uint8_t DS1302::hour() {
  uint8_t hr = read_register(HR_REG);
  uint8_t adj;
  if (hr & 128)  // 12-hour mode
    adj = 12 * ((hr & 32) >> 5);
  else           // 24-hour mode
    adj = 10 * ((hr & (32 + 16)) >> 4);
  hr = (hr & 15) + adj;
  return hr;
}

uint8_t DS1302::date() {
  return _register_bcd_to_dec(DATE_REG, 5);
}

uint8_t DS1302::month() {
  return _register_bcd_to_dec(MON_REG, 4);
}

Time::Day DS1302::day() {
  return static_cast<Time::Day>(_register_bcd_to_dec(DAY_REG, 2));
}

uint16_t DS1302::year() {
  return 2000 + _register_bcd_to_dec(YR_REG);
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
  _register_dec_to_bcd(SEC_REG, sec, 6);
}

void DS1302::minutes(const uint8_t min) {
  _register_dec_to_bcd(MIN_REG, min, 6);
}

void DS1302::hour(const uint8_t hr) {
  write_register(HR_REG, 0);  // set 24-hour mode
  _register_dec_to_bcd(HR_REG, hr, 5);
}

void DS1302::date(const uint8_t date) {
  _register_dec_to_bcd(DATE_REG, date, 5);
}

void DS1302::month(const uint8_t mon) {
  _register_dec_to_bcd(MON_REG, mon, 4);
}

void DS1302::day(const Time::Day day) {
  _register_dec_to_bcd(DAY_REG, static_cast<int>(day), 2);
}

void DS1302::year(uint16_t yr) {
  yr -= 2000;
  _register_dec_to_bcd(YR_REG, yr);
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
