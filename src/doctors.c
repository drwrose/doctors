#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

//#define FAST_TIME 1

#define MY_UUID { 0x22, 0x1E, 0xA6, 0x2F, 0xE2, 0xD0, 0x47, 0x25, 0x97, 0xC3, 0x7F, 0xB3, 0xA2, 0xAF, 0x4C, 0x0C }
PBL_APP_INFO(MY_UUID,
             "12 Doctors", "drwrose",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);

Window window;

Layer face_layer;   // The "face", in both senses (and also the hour indicator).
Layer minute_layer; // The minutes indicator.

int face_value;       // The current face on display (or transitioning into)
bool face_transition; // True if the face is in transition
int transition_start; // Start tick of the current transition, if active

int minute_value;    // The current minute value displayed

int face_resource_ids[12] = {
  RESOURCE_ID_TWELVE,
  RESOURCE_ID_ONE,
  RESOURCE_ID_TWO,
  RESOURCE_ID_THREE,
  RESOURCE_ID_FOUR,
  RESOURCE_ID_FIVE,
  RESOURCE_ID_SIX,
  RESOURCE_ID_SEVEN,
  RESOURCE_ID_EIGHT,
  RESOURCE_ID_NINE,
  RESOURCE_ID_TEN,
  RESOURCE_ID_ELEVEN,
};
  
void face_layer_update_callback(Layer *me, GContext* ctx) {
  BmpContainer image;
  bmp_init_container(face_resource_ids[face_value], &image);

  GRect destination = layer_get_frame(me);
  destination.origin.x = 0;
  destination.origin.y = 0;

  //graphics_context_set_compositing_mode(ctx, GCompOpOr);
  graphics_draw_bitmap_in_rect(ctx, &image.bmp, destination);
  
  bmp_deinit_container(&image);
}
  
void minute_layer_update_callback(Layer *me, GContext* ctx) {
  GFont font;
  GRect box;
  static const int buffer_size = 128;
  char buffer[buffer_size];

  font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);

  box = layer_get_frame(me);
  box.origin.x = 0;
  box.origin.y = 0;

  graphics_context_set_text_color(ctx, GColorBlack);

  snprintf(buffer, buffer_size, ":%02d", minute_value);
  graphics_text_draw(ctx, buffer, font, box,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
                     NULL);
}


// Update the watch as time passes.
void handle_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;
  int face_new;
  int minute_new;

  face_new = t->tick_time->tm_hour % 12;
  minute_new = t->tick_time->tm_min;
#ifdef FAST_TIME
  face_new = ((t->tick_time->tm_min * 60 + t->tick_time->tm_sec) / 5) % 12;
  minute_new = t->tick_time->tm_sec;
#endif

  if (minute_new != minute_value) {
    // Update the minute display.
    minute_value = minute_new;
    layer_mark_dirty(&minute_layer);
  }

  if (face_new != face_value) {
    // Update the face display.
    face_value = face_new;
    layer_mark_dirty(&face_layer);

    // TODO: animate transition.
  }
}

void handle_init(AppContextRef ctx) {
  PblTm time;

  window_init(&window, "12 Doctors");
  window_stack_push(&window, true /* Animated */);

  get_time(&time);
  face_value = time.tm_hour % 12;
  face_transition = false;
  minute_value = time.tm_min;

  resource_init_current_app(&APP_RESOURCES);

  layer_init(&face_layer, window.layer.frame);
  face_layer.update_proc = &face_layer_update_callback;
  layer_add_child(&window.layer, &face_layer);

  layer_init(&minute_layer, GRect(95, 134, 54, 35));
  minute_layer.update_proc = &minute_layer_update_callback;
  layer_add_child(&window.layer, &minute_layer);
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  // Nothing to do right now.
}

void pbl_main(void *params) {
  PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
    .deinit_handler = &handle_deinit,
    .tick_info = {
      .tick_handler = &handle_tick,
#ifdef FAST_TIME
      .tick_units = SECOND_UNIT
#else
      .tick_units = MINUTE_UNIT
#endif
    }
  };
  app_event_loop(params, &handlers);
}
