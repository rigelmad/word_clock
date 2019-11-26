#include <RTClib.h>
#include <Wire.h>

RTC_DS3231 RTC;

void setup() {
  // put your setup code here, to run once:
  RTC.begin();
  RTC.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0, 0, 0, 7));
}

void loop() {
  // put your main code here, to run repeatedly:

}
