#include <pebble.h>
#include <stdio.h>
#include "clock.h"

#define INK GColorFromHEX(0x000055)
#define OCEAN_NAVY GColorFromHEX(0x0055AA)
#define RULE GColorLightGray
#define TEAL GColorFromHEX(0x005555)
#define DAY_FILL GColorFromHEX(0xFFFF55)
#define DAY_EDGE GColorFromHEX(0xAAAA55)
#define SUN GColorFromHEX(0xAA5500)
#define NIGHT_FILL GColorFromHEX(0x000055)
#define NIGHT_EDGE GColorFromHEX(0x5555AA)
#define MOON GColorFromHEX(0xFFFFAA)

static Window *s_window;
static Layer *s_canvas;
static GBitmap *s_wave;
static AtlasClock s_clock;
static bool s_health_subscribed;
static uint32_t s_steps;
static uint8_t s_battery;
static char s_steps_text[12];
static char s_battery_text[5];

enum { FONT_TIME, FONT_WEEKDAY, FONT_DAY, FONT_JAPAN, FONT_LABEL, FONT_SMALL,
       FONT_STATUS, FONT_TINY, FONT_COUNT };
static GFont s_fonts[FONT_COUNT];
static const uint32_t FONT_IDS[FONT_COUNT] = {
  RESOURCE_ID_FONT_TIME_54, RESOURCE_ID_FONT_WEEKDAY_17, RESOURCE_ID_FONT_DAY_35,
  RESOURCE_ID_FONT_JAPAN_18, RESOURCE_ID_FONT_LABEL_11, RESOURCE_ID_FONT_SMALL_9,
  RESOURCE_ID_FONT_STATUS_10, RESOURCE_ID_FONT_TINY_8
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

static void redraw(void) { if (s_canvas) layer_mark_dirty(s_canvas); }

static void japan_icon(GContext *ctx, GColor fill) {
  if (s_clock.japan_day) {
    graphics_context_set_fill_color(ctx, SUN);
    graphics_fill_circle(ctx, GPoint(29,187), 6);
    graphics_context_set_stroke_color(ctx, SUN);
    graphics_context_set_stroke_width(ctx, 2);
    static const GPoint starts[]={{29,174},{29,196},{16,187},{38,187},
                                 {20,178},{35,193},{20,196},{35,181}};
    static const GPoint ends[]={{29,178},{29,200},{20,187},{42,187},
                               {23,181},{38,196},{23,193},{38,178}};
    for (size_t i=0;i<ARRAY_LENGTH(starts);i++) graphics_draw_line(ctx,starts[i],ends[i]);
    graphics_context_set_stroke_width(ctx, 1);
  } else {
    graphics_context_set_fill_color(ctx, MOON);
    graphics_fill_circle(ctx, GPoint(28,187), 11);
    graphics_context_set_fill_color(ctx, fill);
    graphics_fill_circle(ctx, GPoint(34,181), 10);
    graphics_context_set_stroke_color(ctx, MOON);
    graphics_draw_line(ctx,GPoint(39,175),GPoint(39,179));
    graphics_draw_line(ctx,GPoint(37,177),GPoint(41,177));
  }
}

static void canvas_draw(Layer *layer, GContext *ctx) {
  graphics_context_set_fill_color(ctx,GColorWhite);
  graphics_fill_rect(ctx,layer_get_bounds(layer),0,GCornerNone);
  text(ctx,s_clock.zone,FONT_SMALL,INK,GRect(10,6,108,14),GTextAlignmentLeft);
  text(ctx,"DUAL TIME",FONT_TINY,GColorDarkGray,GRect(127,6,63,12),GTextAlignmentRight);
  text(ctx,"PACIFIC",FONT_LABEL,TEAL,GRect(10,26,80,16),GTextAlignmentLeft);
  text(ctx,"PRESS",FONT_LABEL,TEAL,GRect(10,39,80,16),GTextAlignmentLeft);

  graphics_context_set_stroke_color(ctx,RULE);
  graphics_draw_line(ctx,GPoint(99,25),GPoint(99,60));
  text(ctx,s_clock.weekday,FONT_WEEKDAY,OCEAN_NAVY,GRect(105,22,42,27),GTextAlignmentLeft);
  text(ctx,s_clock.month,FONT_SMALL,OCEAN_NAVY,GRect(106,47,40,14),GTextAlignmentLeft);
  text(ctx,s_clock.day,FONT_DAY,OCEAN_NAVY,GRect(147,17,44,47),GTextAlignmentRight);
  text(ctx,s_clock.time,FONT_TIME,OCEAN_NAVY,GRect(8,53,160,68),GTextAlignmentLeft);
  text(ctx,s_clock.period,FONT_LABEL,OCEAN_NAVY,GRect(168,72,23,16),GTextAlignmentRight);

  graphics_context_set_compositing_mode(ctx,GCompOpSet);
  if (s_wave) graphics_draw_bitmap_in_rect(ctx,s_wave,GRect(8,123,184,39));

  const GColor card=s_clock.japan_day?DAY_FILL:NIGHT_FILL;
  const GColor edge=s_clock.japan_day?DAY_EDGE:NIGHT_EDGE;
  const GColor card_text=s_clock.japan_day?INK:GColorWhite;
  graphics_context_set_fill_color(ctx,card);
  graphics_fill_rect(ctx,GRect(10,166,180,41),0,GCornerNone);
  graphics_context_set_stroke_color(ctx,edge);
  graphics_draw_rect(ctx,GRect(10,166,180,41));
  japan_icon(ctx,card);
  text(ctx,s_clock.japan_day?"JST DAY":"JST NIGHT",FONT_TINY,card_text,GRect(47,169,68,12),GTextAlignmentLeft);
  text(ctx,s_clock.japan_date,FONT_TINY,card_text,GRect(114,169,71,12),GTextAlignmentRight);
  text(ctx,s_clock.japan_time,FONT_JAPAN,card_text,GRect(47,179,64,25),GTextAlignmentLeft);
  const int period_x=47+text_width(s_clock.japan_time,FONT_JAPAN)+2;
  text(ctx,s_clock.japan_period,FONT_SMALL,card_text,GRect(period_x,189,20,14),GTextAlignmentLeft);
  text(ctx,s_clock.dst?"+16h":"+17h",FONT_TINY,card_text,GRect(125,192,27,12),GTextAlignmentRight);
  graphics_context_set_stroke_color(ctx,card_text);
  graphics_draw_line(ctx,GPoint(155,194),GPoint(155,202));
  text(ctx,s_clock.japan_next_day?"+1D":"SAME",FONT_TINY,card_text,GRect(158,192,27,12),GTextAlignmentRight);

  graphics_context_set_stroke_color(ctx,RULE);
  graphics_draw_line(ctx,GPoint(0,209),GPoint(199,209));
  graphics_context_set_fill_color(ctx,INK);
  graphics_fill_rect(ctx,GRect(13,214,3,5),0,GCornerNone);
  graphics_fill_rect(ctx,GRect(10,219,4,2),0,GCornerNone);
  graphics_context_set_stroke_color(ctx,INK);
  graphics_draw_line(ctx,GPoint(15,216),GPoint(20,218));
  text(ctx,s_steps_text,FONT_STATUS,INK,GRect(26,210,99,16),GTextAlignmentLeft);
  text(ctx,s_battery_text,FONT_STATUS,INK,GRect(151,210,39,16),GTextAlignmentRight);
  graphics_draw_rect(ctx,GRect(132,216,15,7));
  graphics_fill_rect(ctx,GRect(147,218,2,3),0,GCornerNone);
  int fill=11*s_battery/100;
  if (s_battery&&!fill) fill=1;
  if (fill) graphics_fill_rect(ctx,GRect(134,218,fill,3),0,GCornerNone);
}

static void health_handler(HealthEventType event, void *context) {
  (void)event;(void)context;refresh_health();redraw();
}
static void battery_handler(BatteryChargeState charge) {
  s_battery=charge.charge_percent;
  snprintf(s_battery_text,sizeof(s_battery_text),"%u%%",s_battery);
  redraw();
}
static void tick_handler(struct tm *tick_time,TimeUnits units) {
  (void)tick_time;(void)units;
  if (!atlas_clock_from_utc(time(NULL),&s_clock)) APP_LOG(APP_LOG_LEVEL_ERROR,"Clock conversion failed");
  refresh_health();redraw();
}
static void window_load(Window *window) {
  for (int i=0;i<FONT_COUNT;i++) s_fonts[i]=fonts_load_custom_font(resource_get_handle(FONT_IDS[i]));
  s_wave=gbitmap_create_with_resource(RESOURCE_ID_PACIFIC_WAVE);
  s_canvas=layer_create(layer_get_bounds(window_get_root_layer(window)));
  layer_set_update_proc(s_canvas,canvas_draw);
  layer_add_child(window_get_root_layer(window),s_canvas);
}
static void window_unload(Window *window) {
  (void)window;
  layer_destroy(s_canvas);s_canvas=NULL;
  gbitmap_destroy(s_wave);
  for (int i=0;i<FONT_COUNT;i++) fonts_unload_custom_font(s_fonts[i]);
}
static void init(void) {
  s_window=window_create();
  window_set_background_color(s_window,GColorWhite);
  window_set_window_handlers(s_window,(WindowHandlers){.load=window_load,.unload=window_unload});
  atlas_clock_from_utc(time(NULL),&s_clock);
  refresh_health();battery_handler(battery_state_service_peek());
  tick_timer_service_subscribe(MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT,tick_handler);
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
