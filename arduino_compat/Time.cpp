#include "Time.h"

int     year(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_year + 1900;
}
int     month(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_mon + 1;
}
int     day(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_mday;
}
int     weekday(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_wday + 1;
}
int     hour(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_hour;
}
int     minute(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_min;
}
int     second(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_sec;
}
