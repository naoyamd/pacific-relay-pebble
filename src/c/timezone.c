#include "timezone.h"

// Gregorian weekday, with Sunday = 0.
static int weekday_of_date(int year, int month, int day) {
  static const int month_offsets[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  if (month < 3) {
    year -= 1;
  }
  return (year + year / 4 - year / 100 + year / 400 + month_offsets[month - 1] + day) % 7;
}

static int nth_weekday(int year, int month, int weekday, int occurrence) {
  int first = weekday_of_date(year, month, 1);
  return 1 + (weekday - first + 7) % 7 + (occurrence - 1) * 7;
}

bool sfo_is_dst_utc_fields(int year, int month, int day, int hour, int minute) {
  (void)minute;
  const int start_day = nth_weekday(year, 3, 0, 2);  // second Sunday in March
  const int end_day = nth_weekday(year, 11, 0, 1);   // first Sunday in November

  if (month < 3 || month > 11) {
    return month > 3 && month < 11;
  }
  if (month == 3) {
    return day > start_day || (day == start_day && hour >= 10);  // 02:00 PST = 10:00 UTC
  }
  if (month == 11) {
    return day < end_day || (day == end_day && hour < 9);         // 02:00 PDT = 09:00 UTC
  }
  return true;
}

bool sfo_time_from_utc(time_t utc, struct tm *out, bool *is_dst) {
  if (!out) {
    return false;
  }

  struct tm *utc_fields = gmtime(&utc);
  if (!utc_fields) {
    return false;
  }

  const int year = utc_fields->tm_year + 1900;
  const bool dst = sfo_is_dst_utc_fields(year, utc_fields->tm_mon + 1,
                                         utc_fields->tm_mday, utc_fields->tm_hour,
                                         utc_fields->tm_min);
  const time_t local_epoch = utc + (dst ? -7 * 60 * 60 : -8 * 60 * 60);
  struct tm *local_fields = gmtime(&local_epoch);
  if (!local_fields) {
    return false;
  }

  *out = *local_fields;
  if (is_dst) {
    *is_dst = dst;
  }
  return true;
}
