#include <pebble.h>
#include <stdio.h>
#include "clock.h"

#define INK GColorOxfordBlue
#define WHITE GColorWhite
#define CYAN GColorCyan
#define AMBER GColorChromeYellow
#define GRAY GColorLightGray
#define NIGHT GColorImperialPurple
#define MOON GColorFromHEX(0xAAAAFF)
#define RULE GColorFromHEX(0x005555)

static Window *s_window;
static Layer *s_canvas;
static Layer *s_art;
static GBitmap *s_earth;
static GPath *s_plane;
static GPath *s_moon;
static AtlasClock s_clock;
static bool s_health_subscribed;
static uint32_t s_steps;
static uint8_t s_battery;
static char s_steps_text[12];
static char s_battery_text[5];

enum {FONT_TIME, FONT_WEEKDAY, FONT_DAY, FONT_JAPAN, FONT_ZONE, FONT_STATUS, FONT_SMALL, FONT_TINY, FONT_COUNT};
static GFont s_fonts[FONT_COUNT];
static const uint32_t FONT_IDS[FONT_COUNT] = {
  RESOURCE_ID_FONT_TIME_54, RESOURCE_ID_FONT_WEEKDAY_23, RESOURCE_ID_FONT_DAY_28,
  RESOURCE_ID_FONT_JAPAN_18, RESOURCE_ID_FONT_ZONE_12, RESOURCE_ID_FONT_STATUS_11,
  RESOURCE_ID_FONT_SMALL_9, RESOURCE_ID_FONT_TINY_8
};
static const GPathInfo PLANE = {
  .num_points = 9,
  .points = (GPoint[]) {{99,7},{106,12},{114,11},{109,16},{110,23},{105,18},{100,19},{102,15},{99,7}}
};
static const GPathInfo MOON_PATH = {
  .num_points = 14,
  .points = (GPoint[]) {{183,57},{177,56},{171,58},{168,63},{169,69},{173,74},{179,76},
                      {184,75},{187,72},{182,73},{179,70},{178,66},{179,62},{181,59}}
};

static void text(GContext *ctx, const char *value, int font, GColor color,
                 GRect rect, GTextAlignment alignment) {
  graphics_context_set_text_color(ctx, color);
  graphics_draw_text(ctx, value, s_fonts[font], rect, GTextOverflowModeFill, alignment, NULL);
}

static int text_width(const char *value, int font) {
  return graphics_text_layout_get_content_size(value, s_fonts[font], GRect(0,0,200,80),
      GTextOverflowModeFill, GTextAlignmentLeft).w;
}

static void refresh_health(void) {
#if defined(PBL_HEALTH)
  const time_t now = time(NULL);
  if (health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), now) &
      HealthServiceAccessibilityMaskAvailable) {
    const HealthValue steps = health_service_sum_today(HealthMetricStepCount);
    s_steps = steps > 0 ? (uint32_t)steps : 0;
  } else s_steps = 0;
#else
  s_steps = 0;
#endif
  if (s_steps >= 1000) snprintf(s_steps_text, sizeof(s_steps_text), "%lu,%03lu",
      (unsigned long)(s_steps / 1000), (unsigned long)(s_steps % 1000));
  else snprintf(s_steps_text, sizeof(s_steps_text), "%lu", (unsigned long)s_steps);
}

static void redraw(void) {
  if (s_canvas) layer_mark_dirty(s_canvas);
  if (s_art) layer_mark_dirty(s_art);
}

static void canvas_draw(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx, INK);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  text(ctx, s_clock.zone, FONT_ZONE, CYAN, GRect(10,8,112,16), GTextAlignmentLeft);
  text(ctx, "PACIFIC", FONT_SMALL, GRAY, GRect(132,10,58,13), GTextAlignmentRight);
  text(ctx, s_clock.weekday, FONT_WEEKDAY, WHITE, GRect(10,28,65,30), GTextAlignmentLeft);
  text(ctx, s_clock.day, FONT_DAY, WHITE, GRect(77,24,43,36), GTextAlignmentLeft);
  text(ctx, s_clock.month, FONT_SMALL, GRAY, GRect(151,38,39,13), GTextAlignmentRight);
  text(ctx, s_clock.time, FONT_TIME, WHITE, GRect(7,50,160,64), GTextAlignmentLeft);
  text(ctx, s_clock.period, FONT_ZONE, WHITE, GRect(168,68,26,17), GTextAlignmentLeft);

  graphics_context_set_stroke_color(ctx, RULE);
  graphics_draw_line(ctx, GPoint(0,208), GPoint(199,208));
  graphics_context_set_fill_color(ctx, GRAY);
  graphics_fill_rect(ctx, GRect(13,212,3,5), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(10,217,4,2), 0, GCornerNone);
  graphics_draw_line(ctx, GPoint(15,214), GPoint(20,216));
  text(ctx, s_steps_text, FONT_STATUS, GRAY, GRect(27,207,95,17), GTextAlignmentLeft);
  text(ctx, s_battery_text, FONT_STATUS, GRAY, GRect(151,207,39,17), GTextAlignmentRight);
  graphics_context_set_stroke_color(ctx, GRAY);
  graphics_draw_rect(ctx, GRect(132,214,15,7));
  graphics_fill_rect(ctx, GRect(147,216,2,3), 0, GCornerNone);
  int fill=11*s_battery/100;
  if (s_battery && !fill) fill=1;
  if (fill) graphics_fill_rect(ctx, GRect(134,216,fill,3), 0, GCornerNone);
}

static void art_draw(Layer *layer, GContext *ctx) {
  (void)layer;
  // The 92px art layer clips the globe to the same area as the HTML's SVG viewport.
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  if (s_earth) graphics_draw_bitmap_in_rect(ctx, s_earth, GRect(89,-14,124,124));
  const GColor accent = s_clock.japan_day ? AMBER : MOON;
  const GColor card = s_clock.japan_day ? AMBER : NIGHT;
  const GColor card_text = s_clock.japan_day ? GColorBlack : WHITE;
  static const GPoint route[] = {{18,32},{31,25},{44,19},{58,15},{73,13},
      {88,13},{104,14},{120,17},{138,21},{156,27},{174,35}};
  graphics_context_set_stroke_color(ctx, accent);
  graphics_context_set_stroke_width(ctx, 2);
  for (size_t i=1; i<ARRAY_LENGTH(route); i++) graphics_draw_line(ctx, route[i-1], route[i]);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_context_set_fill_color(ctx, accent);
  graphics_fill_circle(ctx, GPoint(18,32), 3);
  graphics_context_set_fill_color(ctx, CYAN);
  graphics_fill_circle(ctx, GPoint(174,35), 3);
  graphics_context_set_fill_color(ctx, WHITE);
  gpath_draw_filled(ctx, s_plane);
  text(ctx, "HND", FONT_TINY, GRAY, GRect(10,15,30,12), GTextAlignmentLeft);
  text(ctx, "SFO", FONT_TINY, CYAN, GRect(168,19,29,12), GTextAlignmentLeft);

  graphics_context_set_fill_color(ctx, accent);
  if (s_clock.japan_day) {
    graphics_fill_circle(ctx, GPoint(177,66), 9);
    graphics_context_set_stroke_color(ctx, accent);
    static const GPoint rays[] = {{177,52},{177,80},{163,66},{191,66},
                                 {167,56},{187,76},{167,76},{187,56}};
    static const GPoint ends[] = {{177,49},{177,83},{160,66},{194,66},
                                 {165,54},{189,78},{165,78},{189,54}};
    for (size_t i=0; i<ARRAY_LENGTH(rays); i++) graphics_draw_line(ctx,rays[i],ends[i]);
  } else {
    gpath_draw_filled(ctx, s_moon);
    graphics_context_set_fill_color(ctx, WHITE);
    graphics_fill_circle(ctx, GPoint(167,60), 1);
    graphics_fill_circle(ctx, GPoint(191,50), 1);
  }

  // A small JST card stays secondary to the Pacific time and date above.
  graphics_context_set_fill_color(ctx, INK);
  graphics_fill_rect(ctx,GRect(7,41,104,49),0,GCornerNone);
  graphics_context_set_fill_color(ctx,card);
  graphics_fill_rect(ctx,GRect(10,44,98,43),0,GCornerNone);
  graphics_context_set_fill_color(ctx,accent);
  graphics_fill_rect(ctx,GRect(10,44,3,43),0,GCornerNone);
  text(ctx,"HND / JST",FONT_TINY,card_text,GRect(18,46,58,12),GTextAlignmentLeft);
  text(ctx,s_clock.japan_day?"DAY":"NIGHT",FONT_TINY,card_text,GRect(76,46,27,12),GTextAlignmentRight);
  text(ctx,s_clock.japan_time,FONT_JAPAN,card_text,GRect(18,54,61,25),GTextAlignmentLeft);
  const int period_x=18+text_width(s_clock.japan_time,FONT_JAPAN)+2;
  text(ctx,s_clock.japan_period,FONT_SMALL,card_text,GRect(period_x,62,19,14),GTextAlignmentLeft);
  if (s_clock.japan_next_day) text(ctx,"+1",FONT_TINY,card_text,GRect(91,61,13,12),GTextAlignmentRight);
  text(ctx,s_clock.japan_date,FONT_TINY,card_text,GRect(18,77,84,11),GTextAlignmentLeft);
}

static void health_handler(HealthEventType event, void *context) {
  (void)event;(void)context;refresh_health();redraw();
}
static void battery_handler(BatteryChargeState charge) {
  s_battery=charge.charge_percent;
  snprintf(s_battery_text,sizeof(s_battery_text),"%u%%",s_battery);
  redraw();
}
static void tick_handler(struct tm *tick_time, TimeUnits units) {
  (void)tick_time;(void)units;
  if (!atlas_clock_from_utc(time(NULL),&s_clock)) APP_LOG(APP_LOG_LEVEL_ERROR,"Clock conversion failed");
  refresh_health();redraw();
}
static void window_load(Window *window) {
  for (int i=0;i<FONT_COUNT;i++) s_fonts[i]=fonts_load_custom_font(resource_get_handle(FONT_IDS[i]));
  s_earth=gbitmap_create_with_resource(RESOURCE_ID_PACIFIC_EARTH);
  s_plane=gpath_create(&PLANE);s_moon=gpath_create(&MOON_PATH);
  s_canvas=layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_set_update_proc(s_canvas,canvas_draw);
  layer_add_child(window_get_root_layer(window),s_canvas);
  s_art=layer_create(GRect(0,116,200,92));
  layer_set_update_proc(s_art,art_draw);
  layer_add_child(s_canvas,s_art);
}
static void window_unload(Window *window) {
  (void)window;
  layer_destroy(s_art);s_art=NULL;
  layer_destroy(s_canvas);s_canvas=NULL;
  gpath_destroy(s_plane);gpath_destroy(s_moon);
  gbitmap_destroy(s_earth);
  for (int i=0;i<FONT_COUNT;i++) fonts_unload_custom_font(s_fonts[i]);
}
static void init(void) {
  s_window=window_create();
  window_set_background_color(s_window,INK);
  window_set_window_handlers(s_window,(WindowHandlers){.load=window_load,.unload=window_unload});
  atlas_clock_from_utc(time(NULL),&s_clock);
  refresh_health();battery_handler(battery_state_service_peek());
  tick_timer_service_subscribe(MINUTE_UNIT | HOUR_UNIT | DAY_UNIT | MONTH_UNIT | YEAR_UNIT,tick_handler);
  battery_state_service_subscribe(battery_handler);
#if defined(PBL_HEALTH)
  s_health_subscribed=health_service_events_subscribe(health_handler,NULL);
#endif
  window_stack_push(s_window,false);
}
static void deinit(void) {
  tick_timer_service_unsubscribe();battery_state_service_unsubscribe();
#if defined(PBL_HEALTH)
  if (s_health_subscribed) health_service_events_unsubscribe();
#endif
  window_destroy(s_window);
}
int main(void) {init();app_event_loop();deinit();}
