#include <DS1302.h>

namespace {

// Set the appropriate digital I/O pin connections. These are the pin
// assignments for the Arduino as well for as the DS1302 chip. See the DS1302
// datasheet:
//
//   http://datasheets.maximintegrated.com/en/ds/DS1302.pdf
const int kCePin   = 5;  // Chip Enable
const int kIoPin   = 6;  // Input/Output
const int kSclkPin = 7;  // Serial Clock

DS1302 rtc(kCePin, kIoPin, kSclkPin);

}  // namespace

void setup() {
  Serial.begin(9600);

  rtc.writeProtect(false);

  // Clear all RAM bytes.
  for (int i = 0; i < 31; ++i) {
    rtc.writeRam(i, 0x00);
  }

  // Write a string to the RAM.
  static const char kHelloWorld[] = "hello world";
  for (int i = 0; i < sizeof(kHelloWorld) - 1; ++i) {
    rtc.writeRam(i, kHelloWorld[i]);
  }
}

void loop() {
  // Read all of the RAM.
  for (int i = 0; i < 31; ++i) {
    const char c = static_cast<char>(rtc.readRam(i));
    Serial.print("RAM index ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(c);
  }

  delay(3000);
}
