#include <time.h>
#include "Arduino.h"

static void    setTime(int hr,int min,int sec,int day, int month, int yr) {
}

static time_t now() {
  return time(NULL);
}

int     year(time_t t);    // the year for the given time
int     month(time_t t);   // the month for the given time
int     day(time_t t);     // the day for the given time
int     weekday(time_t t); // the weekday for the given time 
int     hour(time_t t);    // the hour for the given time
int     minute(time_t t);  // the minute for the given time
int     second(time_t t);  // the second for the given time
