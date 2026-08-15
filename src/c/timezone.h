#pragma once

#include <pebble.h>

// The rule is expressed in UTC so it is deterministic on the watch and in tests.
bool sfo_is_dst_utc_fields(int year, int month, int day, int hour, int minute);

// Convert a UTC epoch to San Francisco local fields and report whether PDT is active.
bool sfo_time_from_utc(time_t utc, struct tm *out, bool *is_dst);
