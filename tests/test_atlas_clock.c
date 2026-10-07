#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "clock.h"

static void check(time_t utc, const char *zone, const char *time, const char *period,
    const char *weekday, const char *day, const char *month, const char *jtime,
    const char *jperiod, const char *jdate, bool daylight, bool next_day) {
  AtlasClock clock;
  assert(atlas_clock_from_utc(utc,&clock));
  assert(strcmp(clock.zone,zone)==0);
  assert(strcmp(clock.time,time)==0 && strcmp(clock.period,period)==0);
  assert(strcmp(clock.weekday,weekday)==0 && strcmp(clock.day,day)==0 && strcmp(clock.month,month)==0);
  assert(strcmp(clock.japan_time,jtime)==0 && strcmp(clock.japan_period,jperiod)==0);
  assert(strcmp(clock.japan_date,jdate)==0);
  assert(clock.japan_day==daylight && clock.japan_next_day==next_day);
}

int main(void) {
  check(1790988300,"SFO / PDT","5:45","PM","FRI","02","OCT","9:45","AM","10/03 SAT",true,true);
  check(1790955900,"SFO / PDT","8:45","AM","FRI","02","OCT","12:45","AM","10/03 SAT",false,true);
  check(1790971140,"SFO / PDT","12:59","PM","FRI","02","OCT","4:59","AM","10/03 SAT",false,true);
  check(1791010740,"SFO / PDT","11:59","PM","FRI","02","OCT","3:59","PM","10/03 SAT",true,true);
  check(1798790340,"SFO / PST","11:59","PM","THU","31","DEC","4:59","PM","01/01 FRI",true,true);
  // Exact Japanese 06:00/18:00 transitions. November also repeats Pacific 01:00.
  check(1781557140,"SFO / PDT","1:59","PM","MON","15","JUN","5:59","AM","06/16 TUE",false,true);
  check(1781557200,"SFO / PDT","2:00","PM","MON","15","JUN","6:00","AM","06/16 TUE",true,true);
  check(1781600340,"SFO / PDT","1:59","AM","TUE","16","JUN","5:59","PM","06/16 TUE",true,false);
  check(1781600400,"SFO / PDT","2:00","AM","TUE","16","JUN","6:00","PM","06/16 TUE",false,false);
  check(1793523540,"SFO / PDT","1:59","AM","SUN","01","NOV","5:59","PM","11/01 SUN",true,false);
  check(1793523600,"SFO / PST","1:00","AM","SUN","01","NOV","6:00","PM","11/01 SUN",false,false);
  check(1772963940,"SFO / PST","1:59","AM","SUN","08","MAR","6:59","PM","03/08 SUN",false,false);
  check(1772964000,"SFO / PDT","3:00","AM","SUN","08","MAR","7:00","PM","03/08 SUN",false,false);
  assert(!atlas_clock_from_utc(1790988300,NULL));
  puts("Atlas C checks passed: 13 cases, JST colors, AM/PM, dates, DST jumps/fold, year rollover");
}
