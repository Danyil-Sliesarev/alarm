#ifndef _CLOCK_H
#define _CLOCK_H

#include <Arduino.h>

struct dt {
    byte day;
    byte month;
    int year;
    byte hours;
    byte minutes;
    byte seconds;
};

void clock_init();
void set_date(const byte day, const byte month, const int year);
void set_time(const byte hours, const byte minutes, const byte seconds);
void set_datetime(const byte day, const byte month, const int year, const byte hours, const byte minutes, const byte seconds);
byte get_day();
byte get_month();
int get_year();
byte get_hours();
byte get_minutes();
byte get_seconds();
struct dt now();

#endif