#pragma once
#include "timezone.h"

typedef struct {
  struct tm pacific;
  struct tm japan;
  bool dst;
  bool japan_day;
  bool japan_next_day;
  char zone[10];
  char time[6];
  char period[3];
  char weekday[4];
  char day[3];
  char month[4];
  char japan_time[6];
  char japan_period[3];
  char japan_date[10];
} AtlasClock;

bool atlas_clock_from_utc(time_t utc, AtlasClock *out);
