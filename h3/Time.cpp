// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "Time.h"

void setTime(int hr,int min,int sec,int day, int month, int yr) {
  rtc_set_time(hr, min, sec);
  rtc_set_date(day, month, yr);
  rtc_set_weekday((day + month + yr + yr / 4 + yr / 100 - 2) % 7);
}

int     year(time_t t) {
  return rtc_get_year();
}
int     month(time_t t) {
  return rtc_get_month();
}
int     day(time_t t) {
  return rtc_get_day();
}
int     weekday(time_t t) {
  return (rtc_get_weekday() + 1) % 7 + 1;
}
int     hour(time_t t) {
  return rtc_get_hour();
}
int     minute(time_t t) {
  return rtc_get_minute();
}
int     second(time_t t) {
  return rtc_get_second();
}
