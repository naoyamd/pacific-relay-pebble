#include <pebble.h>

#include <stdio.h>
#include <time.h>

#include "timezone.h"

#define COLOR_NAVY GColorFromHEX(0x000055)
#define COLOR_INK GColorFromHEX(0x000033)
#define COLOR_WHITE GColorFromHEX(0xFFFFFF)
#define COLOR_CYAN GColorFromHEX(0x00FFFF)
#define COLOR_CORAL GColorFromHEX(0xFF5555)
#define COLOR_AMBER GColorFromHEX(0xFFAA00)
#define COLOR_GRID GColorFromHEX(0x0000AA)
#define COLOR_ROUTE GColorFromHEX(0x005555)
#define COLOR_NIGHT GColorFromHEX(0x550055)
#define COLOR_DAY GColorFromHEX(0x0055AA)

static Window *s_window;
static Layer *s_canvas_layer;
static bool s_health_subscribed;

static struct tm s_jst;
static struct tm s_sfo;
static bool s_sfo_dst;
static bool s_sfo_day;

static uint32_t s_steps;
static uint8_t s_battery;

static char s_jst_time[8];
static char s_jst_period[3];
static char s_jst_date[16];
static char s_sfo_time[8];
static char s_sfo_period[3];
static char s_sfo_zone[12];
static char s_sfo_date[16];
static char s_steps_text[12];
static char s_battery_text[8];

static const char *const MONTHS[] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

static const char *const WEEKDAYS[] = {
  "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static void format_time_12(const struct tm *value, char *time_text, size_t time_size,
                           char *period_text, size_t period_size) {
  int hour = value->tm_hour % 12;
  if (hour == 0) {
    hour = 12;
  }
  snprintf(time_text, time_size, "%d:%02d", hour, value->tm_min);
  snprintf(period_text, period_size, "%s", value->tm_hour >= 12 ? "PM" : "AM");
}

static void format_date(const struct tm *value, char *text, size_t size) {
  snprintf(text, size, "%s %02d %s", MONTHS[value->tm_mon], value->tm_mday,
           WEEKDAYS[value->tm_wday]);
}

static void format_steps(void) {
  const unsigned long steps = (unsigned long)s_steps;
  if (steps >= 1000) {
    snprintf(s_steps_text, sizeof(s_steps_text), "%lu,%03lu", steps / 1000, steps % 1000);
  } else {
    snprintf(s_steps_text, sizeof(s_steps_text), "%lu", steps);
  }
}

static void format_battery(void) {
  snprintf(s_battery_text, sizeof(s_battery_text), "%u%%", (unsigned)s_battery);
}

static void refresh_time(void) {
  const time_t now = time(NULL);
  const time_t jst_epoch = now + 9 * 60 * 60;
  struct tm *jst = gmtime(&jst_epoch);
  if (jst) {
    s_jst = *jst;
  }

  sfo_time_from_utc(now, &s_sfo, &s_sfo_dst);
  s_sfo_day = s_sfo.tm_hour >= 6 && s_sfo.tm_hour < 18;

  format_time_12(&s_jst, s_jst_time, sizeof(s_jst_time), s_jst_period, sizeof(s_jst_period));
  format_date(&s_jst, s_jst_date, sizeof(s_jst_date));
  format_time_12(&s_sfo, s_sfo_time, sizeof(s_sfo_time), s_sfo_period, sizeof(s_sfo_period));
  format_date(&s_sfo, s_sfo_date, sizeof(s_sfo_date));
  snprintf(s_sfo_zone, sizeof(s_sfo_zone), "SFO / %s", s_sfo_dst ? "PDT" : "PST");
}

static void refresh_health(void) {
#if defined(PBL_HEALTH)
  const time_t now = time(NULL);
  const time_t start = time_start_of_today();
  if (health_service_metric_accessible(HealthMetricStepCount, start, now) &
      HealthServiceAccessibilityMaskAvailable) {
    const HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    s_steps = steps > 0 ? (uint32_t)steps : 0;
  } else {
    s_steps = 0;
  }
#else
  s_steps = 0;
#endif
  format_steps();
}

static void draw_polyline(GContext *ctx, const GPoint *points, uint8_t count, bool close) {
  for (uint8_t i = 1; i < count; i++) {
    graphics_draw_line(ctx, points[i - 1], points[i]);
  }
  if (close && count > 1) {
    graphics_draw_line(ctx, points[count - 1], points[0]);
  }
}

static void draw_dashed_vertical(GContext *ctx, int x, int y_start, int y_end) {
  for (int y = y_start; y < y_end; y += 4) {
    graphics_draw_line(ctx, GPoint(x, y), GPoint(x, y + 2));
  }
}

static void draw_globe(GContext *ctx) {
  static const GPoint meridian_wide[] = {
    GPoint(155, 82), GPoint(187, 92), GPoint(211, 120), GPoint(219, 170),
    GPoint(211, 220), GPoint(187, 248), GPoint(155, 258), GPoint(123, 248),
    GPoint(99, 220), GPoint(91, 170), GPoint(99, 120), GPoint(123, 92)
  };
  static const GPoint meridian_narrow[] = {
    GPoint(155, 82), GPoint(170, 93), GPoint(181, 120), GPoint(185, 170),
    GPoint(181, 220), GPoint(170, 247), GPoint(155, 258), GPoint(140, 247),
    GPoint(129, 220), GPoint(125, 170), GPoint(129, 120), GPoint(140, 93)
  };
  static const GPoint latitude_wide[] = {
    GPoint(88, 113), GPoint(105, 102), GPoint(122, 96), GPoint(139, 94),
    GPoint(155, 93), GPoint(171, 94), GPoint(188, 96), GPoint(205, 102),
    GPoint(222, 113)
  };
  static const GPoint latitude_wide_bottom[] = {
    GPoint(88, 227), GPoint(105, 238), GPoint(122, 244), GPoint(139, 246),
    GPoint(155, 247), GPoint(171, 246), GPoint(188, 244), GPoint(205, 238),
    GPoint(222, 227)
  };
  static const GPoint latitude_high[] = {
    GPoint(105, 98), GPoint(117, 91), GPoint(130, 87), GPoint(142, 86),
    GPoint(155, 85), GPoint(168, 86), GPoint(180, 87), GPoint(193, 91),
    GPoint(205, 98)
  };
  static const GPoint latitude_high_bottom[] = {
    GPoint(105, 242), GPoint(117, 249), GPoint(130, 253), GPoint(142, 254),
    GPoint(155, 255), GPoint(168, 254), GPoint(180, 253), GPoint(193, 249),
    GPoint(205, 242)
  };
  static const GPoint latitude_low[] = {
    GPoint(80, 124), GPoint(99, 118), GPoint(118, 114), GPoint(137, 113),
    GPoint(155, 112), GPoint(173, 113), GPoint(192, 114), GPoint(211, 118),
    GPoint(230, 124)
  };
  static const GPoint latitude_low_bottom[] = {
    GPoint(80, 216), GPoint(99, 222), GPoint(118, 226), GPoint(137, 227),
    GPoint(155, 228), GPoint(173, 227), GPoint(192, 226), GPoint(211, 222),
    GPoint(230, 216)
  };
  static const GPoint latitude_narrow[] = {
    GPoint(72, 141), GPoint(93, 136), GPoint(114, 133), GPoint(135, 132),
    GPoint(155, 132), GPoint(175, 132), GPoint(196, 133), GPoint(217, 136),
    GPoint(238, 141)
  };
  static const GPoint latitude_narrow_bottom[] = {
    GPoint(72, 199), GPoint(93, 204), GPoint(114, 207), GPoint(135, 208),
    GPoint(155, 208), GPoint(175, 208), GPoint(196, 207), GPoint(217, 204),
    GPoint(238, 199)
  };

  graphics_context_set_stroke_color(ctx, COLOR_DAY);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, GPoint(155, 170), 88);
  draw_polyline(ctx, meridian_wide, sizeof(meridian_wide) / sizeof(meridian_wide[0]), true);
  draw_polyline(ctx, meridian_narrow, sizeof(meridian_narrow) / sizeof(meridian_narrow[0]), true);
  draw_polyline(ctx, latitude_wide, sizeof(latitude_wide) / sizeof(latitude_wide[0]), false);
  draw_polyline(ctx, latitude_wide_bottom, sizeof(latitude_wide_bottom) / sizeof(latitude_wide_bottom[0]), false);
  draw_polyline(ctx, latitude_high, sizeof(latitude_high) / sizeof(latitude_high[0]), false);
  draw_polyline(ctx, latitude_high_bottom, sizeof(latitude_high_bottom) / sizeof(latitude_high_bottom[0]), false);
  draw_polyline(ctx, latitude_low, sizeof(latitude_low) / sizeof(latitude_low[0]), false);
  draw_polyline(ctx, latitude_low_bottom, sizeof(latitude_low_bottom) / sizeof(latitude_low_bottom[0]), false);
  draw_polyline(ctx, latitude_narrow, sizeof(latitude_narrow) / sizeof(latitude_narrow[0]), false);
  draw_polyline(ctx, latitude_narrow_bottom, sizeof(latitude_narrow_bottom) / sizeof(latitude_narrow_bottom[0]), false);
  graphics_context_set_stroke_width(ctx, 1);
}

static void draw_route(GContext *ctx) {
  static const GPoint route[] = {
    GPoint(16, 98), GPoint(32, 93), GPoint(48, 89), GPoint(68, 86),
    GPoint(100, 83), GPoint(132, 86), GPoint(152, 89), GPoint(168, 93),
    GPoint(184, 98)
  };

  graphics_context_set_stroke_color(ctx, COLOR_ROUTE);
  draw_polyline(ctx, route, sizeof(route) / sizeof(route[0]), false);

  graphics_context_set_stroke_color(ctx, COLOR_AMBER);
  draw_dashed_vertical(ctx, 100, 80, 102);

  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_circle(ctx, GPoint(16, 98), 3);
  graphics_context_set_fill_color(ctx, COLOR_WHITE);
  graphics_fill_circle(ctx, GPoint(48, 89), 2);
  graphics_context_set_fill_color(ctx, COLOR_AMBER);
  graphics_fill_circle(ctx, GPoint(100, 83), 2);
  graphics_context_set_fill_color(ctx, COLOR_WHITE);
  graphics_fill_circle(ctx, GPoint(152, 89), 2);
  graphics_context_set_fill_color(ctx, COLOR_CYAN);
  graphics_fill_circle(ctx, GPoint(184, 98), 3);

  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, "HND", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(9, 102, 44, 15), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_draw_text(ctx, "SFO", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(147, 102, 44, 15), GTextOverflowModeFill, GTextAlignmentRight, NULL);
  graphics_context_set_text_color(ctx, COLOR_AMBER);
  graphics_draw_text(ctx, "DATE LINE", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(65, 88, 70, 15), GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void draw_header(GContext *ctx) {
  graphics_context_set_text_color(ctx, COLOR_AMBER);
  graphics_draw_text(ctx, "PACIFIC", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(9, 3, 82, 16), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, GColorFromHEX(0xAAAAAA));
  graphics_draw_text(ctx, "PR-082 HND>SFO", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(93, 3, 98, 16), GTextOverflowModeFill, GTextAlignmentRight, NULL);

  graphics_context_set_text_color(ctx, COLOR_CORAL);
  graphics_draw_text(ctx, "HND / JST", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(9, 19, 82, 16), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, s_jst_date, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(117, 19, 74, 16), GTextOverflowModeFill, GTextAlignmentRight, NULL);

  const GFont time_font = fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49);
  const GRect time_frame = GRect(8, 27, 180, 58);
  const GSize time_size = graphics_text_layout_get_content_size(
      s_jst_time, time_font, time_frame, GTextOverflowModeFill, GTextAlignmentLeft);
  int period_x = 8 + time_size.w + 2;
  if (period_x > 162) {
    period_x = 162;
  }
  graphics_draw_text(ctx, s_jst_time, time_font, time_frame,
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, COLOR_CORAL);
  graphics_draw_text(ctx, s_jst_period, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(period_x, 35, 38, 18), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

static void draw_sfo_strip(GContext *ctx) {
  const GRect inner = GRect(9, 129, 182, 40);
  const int left_width = 112;
  const GColor left_color = s_sfo_day ? COLOR_DAY : COLOR_NIGHT;
  const GColor right_color = s_sfo_day ? COLOR_AMBER : COLOR_INK;
  const GColor border_color = s_sfo_day ? COLOR_CYAN : COLOR_GRID;
  const GColor rail_color = s_sfo_day ? COLOR_AMBER : COLOR_GRID;
  const GColor right_text = s_sfo_day ? COLOR_INK : COLOR_WHITE;

  graphics_context_set_fill_color(ctx, left_color);
  graphics_fill_rect(ctx, GRect(inner.origin.x, inner.origin.y, left_width, inner.size.h), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, right_color);
  graphics_fill_rect(ctx, GRect(inner.origin.x + left_width, inner.origin.y,
                                inner.size.w - left_width, inner.size.h), 0, GCornerNone);
  graphics_context_set_stroke_color(ctx, border_color);
  graphics_draw_rect(ctx, GRect(8, 128, 184, 42));
  graphics_context_set_fill_color(ctx, rail_color);
  graphics_fill_rect(ctx, GRect(9, 129, 3, 40), 0, GCornerNone);

  graphics_context_set_text_color(ctx, COLOR_CYAN);
  graphics_draw_text(ctx, s_sfo_zone, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(17, 130, 102, 16), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, right_text);
  graphics_draw_text(ctx, "LOCAL", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(140, 130, 45, 16), GTextOverflowModeFill, GTextAlignmentRight, NULL);

  const GFont sfo_time_font = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  const GRect sfo_time_frame = GRect(17, 139, 102, 29);
  const GSize sfo_time_size = graphics_text_layout_get_content_size(
      s_sfo_time, sfo_time_font, sfo_time_frame, GTextOverflowModeFill, GTextAlignmentLeft);
  const int sfo_period_x = 17 + sfo_time_size.w + 2;
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, s_sfo_time, sfo_time_font, sfo_time_frame,
                     GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, COLOR_CORAL);
  graphics_draw_text(ctx, s_sfo_period, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(sfo_period_x, 143, 24, 16), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
  graphics_context_set_text_color(ctx, right_text);
  graphics_draw_text(ctx, s_sfo_date, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(121, 153, 70, 16), GTextOverflowModeFill, GTextAlignmentRight, NULL);
}

static void draw_footsteps(GContext *ctx) {
  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_circle(ctx, GPoint(22, 206), 3);
  graphics_fill_circle(ctx, GPoint(28, 199), 3);
  graphics_fill_circle(ctx, GPoint(18, 200), 1);
  graphics_fill_circle(ctx, GPoint(23, 196), 1);
  graphics_fill_circle(ctx, GPoint(28, 193), 1);
  graphics_fill_circle(ctx, GPoint(33, 196), 1);
}

static void draw_status_band(GContext *ctx) {
  graphics_context_set_fill_color(ctx, COLOR_INK);
  graphics_fill_rect(ctx, GRect(0, 184, 200, 34), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, COLOR_CORAL);
  graphics_fill_rect(ctx, GRect(0, 184, 3, 34), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, COLOR_CYAN);
  graphics_fill_rect(ctx, GRect(197, 184, 3, 34), 0, GCornerNone);

  draw_footsteps(ctx);
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, s_steps_text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(42, 191, 66, 20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);

  graphics_context_set_stroke_color(ctx, COLOR_CYAN);
  graphics_draw_rect(ctx, GRect(122, 194, 29, 14));
  graphics_context_set_fill_color(ctx, COLOR_CYAN);
  graphics_fill_rect(ctx, GRect(151, 198, 3, 6), 0, GCornerNone);
  graphics_context_set_fill_color(ctx, COLOR_AMBER);
  int fill_width = (int)(24 * s_battery / 100);
  if (fill_width < 1 && s_battery > 0) {
    fill_width = 1;
  }
  graphics_fill_rect(ctx, GRect(125, 197, fill_width, 8), 0, GCornerNone);
  graphics_context_set_text_color(ctx, COLOR_WHITE);
  graphics_draw_text(ctx, s_battery_text, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD),
                     GRect(158, 191, 34, 20), GTextOverflowModeFill, GTextAlignmentLeft, NULL);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  const GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, COLOR_NAVY);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  draw_globe(ctx);
  draw_header(ctx);
  draw_route(ctx);
  draw_sfo_strip(ctx);
  draw_status_band(ctx);
}

static void health_handler(HealthEventType event, void *context) {
  (void)event;
  (void)context;
  refresh_health();
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void battery_handler(BatteryChargeState charge) {
  s_battery = charge.charge_percent;
  format_battery();
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  (void)tick_time;
  (void)units_changed;
  refresh_time();
  refresh_health();
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  s_canvas_layer = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);
  layer_add_child(root, s_canvas_layer);
}

static void window_unload(Window *window) {
  (void)window;
  layer_destroy(s_canvas_layer);
  s_canvas_layer = NULL;
}

static void init(void) {
  s_window = window_create();
  window_set_background_color(s_window, COLOR_NAVY);
  window_set_window_handlers(s_window, (WindowHandlers){
    .load = window_load,
    .unload = window_unload,
  });

  s_battery = battery_state_service_peek().charge_percent;
  format_battery();
  refresh_time();
  refresh_health();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
#if defined(PBL_HEALTH)
  s_health_subscribed = health_service_events_subscribe(health_handler, NULL);
#endif

  window_stack_push(s_window, false);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
#if defined(PBL_HEALTH)
  if (s_health_subscribed) {
    health_service_events_unsubscribe();
  }
#endif
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
