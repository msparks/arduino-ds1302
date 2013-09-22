// Interface for the DS1302 timekeeping chip.
//
// Copyright (c) 2009, Matt Sparks
// All rights reserved.
//
// Distributed under the 2-clause BSD license.
#ifndef DS1302_H_
#define DS1302_H_

// Convenience register constants.
#define SEC_REG  0
#define MIN_REG  1
#define HR_REG   2
#define DATE_REG 3
#define MON_REG  4
#define DAY_REG  5
#define YR_REG   6
#define WP_REG   7

// Type for a register number.
typedef uint8_t reg_t;

// Class representing a particular time and date.
class Time {
 public:
  // Creates a time object dated to Saturday Jan 1, 2000 at 00:00:00.
  // The date and time can be changed by editing the instance variables.
  Time();

  // Creates a Time object with a given time.
  //
  // Args:
  //   yr: year. Range: {2000, ..., 2099}.
  //   mon: month. Range: {1, ..., 12}.
  //   date: date (of the month). Range: {1, ..., 31}.
  //   hr: hour. Range: {0, ..., 23}.
  //   min: minutes. Range: {0, ..., 59}.
  //   sec: seconds. Range: {0, ..., 59}.
  //   day: day of the week. Sunday is 1. Range: {1, ..., 7}.
  Time(uint16_t yr, uint8_t mon, uint8_t date,
       uint8_t hr, uint8_t min, uint8_t sec,
       uint8_t day);

  uint8_t sec;
  uint8_t min;
  uint8_t hr;
  uint8_t date;
  uint8_t mon;
  uint8_t day;
  uint16_t yr;
};


// Talks to a Dallas Semiconductor DS1302 Real Time Clock (RTC) chip.
class DS1302 {
 public:
  // Prepares to interface with the chip on the given I/O pins.
  //
  // Args:
  //   ce_pin: CE pin number
  //   io_pin: IO pin number
  //   sclk_pin: SCLK pin number
  DS1302(uint8_t ce_pin, uint8_t io_pin, uint8_t sclk_pin);

  // Reads register byte value.
  //
  // Args:
  //   reg: register number
  //
  // Returns:
  //   register value
  uint8_t read_register(reg_t reg);

  // Writes byte into register.
  //
  // Args:
  //   reg: register number
  //   value: byte to write
  void write_register(reg_t reg, uint8_t value);

  // Enables or disables write protection on chip.
  //
  // Args:
  //   enable: true to enable, false to disable.
  void write_protect(bool enable);

  // Set or clear clock halt flag.
  //
  // Args:
  //   value: true to set halt flag, false to clear.
  void halt(bool value);

  // Returns an individual piece of the time and date.
  uint8_t seconds();
  uint8_t minutes();
  uint8_t hour();
  uint8_t date();
  uint8_t month();
  uint8_t day();
  uint16_t year();

  // Get the current time and date in a Time object.
  //
  // Returns:
  //   Current time as Time object.
  Time time();

  // Individually sets pieces of the date and time.
  //
  // The arguments here follow the rules specified above in Time::Time(...).
  void seconds(uint8_t sec);
  void minutes(uint8_t min);
  void hour(uint8_t hr);
  void date(uint8_t date);
  void month(uint8_t mon);
  void day(uint8_t day);
  void year(uint16_t yr);

  // Sets the time and date to the instant specified in a given Time object.
  //
  // Args:
  //   t: Time object to use
  void time(Time t);

private:
  uint8_t _ce_pin;
  uint8_t _io_pin;
  uint8_t _sclk_pin;

  // Shifts out a value to the IO pin.
  //
  // Side effects: sets _io_pin as OUTPUT.
  //
  // Args:
  //   value: byte to shift out
  void _write_out(uint8_t value);

  // Reads in a byte from the IO pin.
  //
  // Side effects: sets _io_pin to INPUT.
  //
  // Returns:
  //   byte read in
  uint8_t _read_in();

  // Gets a binary-coded decimal register and returns it in decimal.
  //
  // Args:
  //   reg: register number
  //   high_bit: number of the bit containing the last BCD value ({0, ..., 7})
  //
  // Returns:
  //   decimal value
  uint8_t _register_bcd_to_dec(reg_t reg, uint8_t high_bit);
  uint8_t _register_bcd_to_dec(reg_t reg);

  // Sets a register with binary-coded decimal converted from a given value.
  //
  // Args:
  //   reg: register number
  //   value: decimal value to convert to BCD
  //   high_bit: highest bit in the register allowed to contain BCD value
  void _register_dec_to_bcd(reg_t reg, uint8_t value, uint8_t high_bit);
  void _register_dec_to_bcd(reg_t reg, uint8_t value);
};

#endif  // DS1302_H_
