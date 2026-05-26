#include <Arduino.h>
#include <EEPROM.h>
#include <RtcDS1302.h>
#include "config.h"
#include "rtc_wrapper.h"

ThreeWire myWire(RTC_DAT, RTC_CLK, RTC_RST);
RtcDS1302<ThreeWire> rtc(myWire);

static struct dt rtc_datetime;

void clock_init() {
    rtc.Begin();
    rtc.SetIsWriteProtected(false);
    rtc.SetIsRunning(true);

    RtcDateTime dt = rtc.GetDateTime();

    if (!rtc.IsDateTimeValid() ||
        dt.Hour() > 23 || dt.Minute() > 59 || dt.Second() > 59 ||
        dt.Day() < 1 || dt.Day() > 31 || dt.Month() < 1 || dt.Month() > 12) {
        RtcDateTime def(2026, 5, 21, 12, 0, 0);
        rtc.SetDateTime(def);
        delay(100);
        dt = rtc.GetDateTime();
    }

    rtc_datetime.hours = dt.Hour();
    rtc_datetime.minutes = dt.Minute();
    rtc_datetime.seconds = dt.Second();
    rtc_datetime.day = dt.Day();
    rtc_datetime.month = dt.Month();
    rtc_datetime.year = dt.Year();
}

void set_date(const byte day, const byte month, const int year) {
    rtc.SetIsWriteProtected(false);
    RtcDateTime dt = rtc.GetDateTime();
    RtcDateTime newdt(year, month, day, dt.Hour(), dt.Minute(), dt.Second());
    rtc.SetDateTime(newdt);
}

void set_time(const byte hours, const byte minutes, const byte seconds) {
    rtc.SetIsWriteProtected(false);
    RtcDateTime dt = rtc.GetDateTime();
    RtcDateTime newdt(dt.Year(), dt.Month(), dt.Day(), hours, minutes, seconds);
    rtc.SetDateTime(newdt);
}

void set_datetime(const byte day, const byte month, const int year,
                  const byte hours, const byte minutes, const byte seconds) {
    rtc.SetIsWriteProtected(false);
    RtcDateTime dt(year, month, day, hours, minutes, seconds);
    rtc.SetDateTime(dt);
}

byte get_day()     { return rtc_datetime.day; }
byte get_month()   { return rtc_datetime.month; }
int  get_year()    { return rtc_datetime.year; }
byte get_hours()   { return rtc_datetime.hours; }
byte get_minutes() { return rtc_datetime.minutes; }
byte get_seconds() { return rtc_datetime.seconds; }

struct dt now() {
    RtcDateTime dt = rtc.GetDateTime();

    if (dt.Hour() <= 23 && dt.Minute() <= 59 && dt.Second() <= 59 &&
        dt.Day() >= 1 && dt.Day() <= 31 && dt.Month() >= 1 && dt.Month() <= 12) {
        rtc_datetime.hours = dt.Hour();
        rtc_datetime.minutes = dt.Minute();
        rtc_datetime.seconds = dt.Second();
        rtc_datetime.day = dt.Day();
        rtc_datetime.month = dt.Month();
        rtc_datetime.year = dt.Year();
    }

    return rtc_datetime;
}