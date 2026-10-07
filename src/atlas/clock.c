#include "clock.h"
#include <stdio.h>

static const char *const MONTHS[] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};
static const char *const WEEKDAYS[] = {
  "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static void format_time(const struct tm *value, char *text, char *period) {
  const unsigned hour = (unsigned)value->tm_hour % 12;
  const unsigned minute = (unsigned)value->tm_min % 60;
  snprintf(text, 6, "%u:%02u", hour ? hour : 12, minute);
  snprintf(period, 3, "%s", value->tm_hour >= 12 ? "PM" : "AM");
}

bool atlas_clock_from_utc(time_t utc, AtlasClock *out) {
  if (!out) return false;
  AtlasClock next = {0};
  if (!sfo_time_from_utc(utc, &next.pacific, &next.dst)) return false;
  const time_t japan_epoch = utc + 9 * 60 * 60;
  const struct tm *japan = gmtime(&japan_epoch);
  if (!japan) return false;
  next.japan = *japan;
  next.japan_day = japan->tm_hour >= 6 && japan->tm_hour < 18;
  next.japan_next_day = japan->tm_year != next.pacific.tm_year ||
      japan->tm_mon != next.pacific.tm_mon || japan->tm_mday != next.pacific.tm_mday;
  snprintf(next.zone, sizeof(next.zone), "SFO / %s", next.dst ? "PDT" : "PST");
  format_time(&next.pacific, next.time, next.period);
  snprintf(next.weekday, sizeof(next.weekday), "%s", WEEKDAYS[next.pacific.tm_wday]);
  snprintf(next.day, sizeof(next.day), "%02d", next.pacific.tm_mday);
  snprintf(next.month, sizeof(next.month), "%s", MONTHS[next.pacific.tm_mon]);
  format_time(japan, next.japan_time, next.japan_period);
  snprintf(next.japan_date, sizeof(next.japan_date), "%02u/%02u %.3s",
           (unsigned)japan->tm_mon % 12 + 1, (unsigned)japan->tm_mday % 32,
           WEEKDAYS[japan->tm_wday]);
  *out = next;
  return true;
}
