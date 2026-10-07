// Host check: compile with a pebble.h shim containing stdbool.h and time.h.
#include <assert.h>
#include <stdio.h>
#include "timezone.h"

static void check(time_t utc, int year, int month, int day, int hour, int minute, bool dst) {
  struct tm sfo;
  bool actual_dst;
  assert(sfo_time_from_utc(utc, &sfo, &actual_dst));
  assert(sfo.tm_year + 1900 == year && sfo.tm_mon + 1 == month);
  assert(sfo.tm_mday == day && sfo.tm_hour == hour && sfo.tm_min == minute);
  assert(actual_dst == dst);
  time_t jst_epoch = utc + 9 * 60 * 60;
  struct tm *jst = gmtime(&jst_epoch);
  assert(jst);
  assert((jst->tm_hour - sfo.tm_hour + 24) % 24 == (dst ? 16 : 17));
}

int main(void) {
  // Actual conversion immediately before/after both 2026 DST transitions.
  check(1772963940, 2026, 3, 8, 1, 59, false);
  check(1772964000, 2026, 3, 8, 3, 0, true);
  check(1793523540, 2026, 11, 1, 1, 59, true);
  check(1793523600, 2026, 11, 1, 1, 0, false);
  // Japan is already in the next day/year while Pacific is still Dec 31.
  check(1767225600, 2025, 12, 31, 16, 0, false);
  assert(!sfo_time_from_utc(1767225600, NULL, NULL));
  puts("Native C timezone checks passed: DST jumps, repeated hour, year rollover, JST offset");
}
