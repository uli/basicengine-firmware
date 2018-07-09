#include "Time.h"

int     year(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_year;
}
int     month(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_mon;
}
int     day(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_mday;
}
int     weekday(time_t t) {
  struct tm *tm = localtime(&t);
  return tm->tm_wday;
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
