#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

#define FAST_TIME 1

#define MY_UUID { 0x22, 0x1E, 0xA6, 0x2F, 0xE2, 0xD0, 0x47, 0x25, 0x97, 0xC3, 0x7F, 0xB3, 0xA2, 0xAF, 0x4C, 0x0C }
PBL_APP_INFO(MY_UUID,
             "12 Doctors", "drwrose",
             1, 0, /* App version */
             DEFAULT_MENU_ICON,
             APP_INFO_WATCH_FACE);


#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

// Number of milliseconds per animation frame
#define ANIM_TICK_MS 50

// Number of frames of animation
#define NUM_TRANSITION_FRAMES 24

// Number of frames for which the tardis is partially offscreen on
// either left or right
#define NUM_FRAMES_OFFSCREEN 5

AppContextRef app_ctx;
Window window;

Layer face_layer;   // The "face", in both senses (and also the hour indicator).
Layer minute_layer; // The minutes indicator.

int face_value;       // The current face on display (or transitioning into)
bool face_transition; // True if the face is in transition
int transition_start; // Start tick of the current transition, if active
int prev_face_value;  // The face we're transitioning from
AppTimerHandle anim_timer = APP_TIMER_INVALID_HANDLE;

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

typedef struct {
  int white;
  int black;
  bool flip_x;
} TardisFrame;

#define NUM_TARDIS_FRAMES 7
TardisFrame tardis_frames[NUM_TARDIS_FRAMES] = {
  { RESOURCE_ID_TARDIS_01_WHITE, RESOURCE_ID_TARDIS_01_BLACK, false },
  { RESOURCE_ID_TARDIS_02_WHITE, RESOURCE_ID_TARDIS_02_BLACK, false },
  { RESOURCE_ID_TARDIS_03_WHITE, RESOURCE_ID_TARDIS_03_BLACK, false },
  { RESOURCE_ID_TARDIS_04_WHITE, RESOURCE_ID_TARDIS_04_BLACK, false },
  { RESOURCE_ID_TARDIS_04_WHITE, RESOURCE_ID_TARDIS_04_BLACK, true },
  { RESOURCE_ID_TARDIS_03_WHITE, RESOURCE_ID_TARDIS_03_BLACK, true },
  { RESOURCE_ID_TARDIS_02_WHITE, RESOURCE_ID_TARDIS_02_BLACK, true }
};

// Reverse the bits of a byte.
// http://www-graphics.stanford.edu/~seander/bithacks.html#BitReverseTable
uint8_t reverse_bits(uint8_t b) {
  return ((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16; 
}

// Horizontally flips the indicated BmpContainer in-place.  Requires
// that the width be a multiple of 8 pixels.
void flip_bitmap_x(BmpContainer *image) {
  int height = image->bmp.bounds.size.h;
  int width = image->bmp.bounds.size.w;  // multiple of 8, by our convention.
  int width_bytes = width / 8;
  int stride = image->bmp.row_size_bytes; // multiple of 4, by Pebble.
  uint8_t *data = image->bmp.addr;

  for (int y = 0; y < height; ++y) {
    uint8_t *row = data + y * stride;
    for (int x1 = (width_bytes - 1) / 2; x1 >= 0; --x1) {
      int x2 = width_bytes - 1 - x1;
      uint8_t b = reverse_bits(row[x1]);
      row[x1] = reverse_bits(row[x2]);
      row[x2] = b;
    }
  }
}

// Returns a number which increments once for each new animation frame.
int get_anim_ticks() {
  time_t s;
  uint16_t ms;

  // ANIM_TICK_MS per frame.
  time_ms(&s, &ms);
  return s * (1000 / ANIM_TICK_MS) + ms / ANIM_TICK_MS;
}

// Ensures the animation timer will fire.
void set_anim_timer() {
  if (anim_timer != APP_TIMER_INVALID_HANDLE) {
    app_timer_cancel_event(app_ctx, anim_timer);
    anim_timer = APP_TIMER_INVALID_HANDLE;
  }
  anim_timer = app_timer_send_event(app_ctx, ANIM_TICK_MS, 0);
}

void clear_anim_timer() {
  if (anim_timer != APP_TIMER_INVALID_HANDLE) {
    app_timer_cancel_event(app_ctx, anim_timer);
    anim_timer = APP_TIMER_INVALID_HANDLE;
  }
}
  
void face_layer_update_callback(Layer *me, GContext* ctx) {
  if (!face_transition) {
    // The simple case: hold a particular frame.
    BmpContainer image;
    bmp_init_container(face_resource_ids[face_value], &image);
    
    GRect destination = layer_get_frame(me);
    destination.origin.x = 0;
    destination.origin.y = 0;
    
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, &image.bmp, destination);
    
    bmp_deinit_container(&image);

  } else {
    // The complex case: we animate a transition from one face to another.
    int ti, wipe_x;
    int tardis_frame;
    BmpContainer prev_image, curr_image, tardis_white, tardis_black;

    // ti ranges from 0 to NUM_TRANSITION_FRAMES over the transition.
    ti = get_anim_ticks() - transition_start;
    if (ti >= NUM_TRANSITION_FRAMES) {
      ti = NUM_TRANSITION_FRAMES;
      face_transition = false;
    }
    wipe_x = SCREEN_WIDTH - (ti - NUM_FRAMES_OFFSCREEN) * SCREEN_WIDTH / (NUM_TRANSITION_FRAMES - NUM_FRAMES_OFFSCREEN * 2);
    tardis_frame = ti % NUM_TARDIS_FRAMES;

    bmp_init_container(face_resource_ids[prev_face_value], &prev_image);
    bmp_init_container(face_resource_ids[face_value], &curr_image);
    bmp_init_container(tardis_frames[tardis_frame].white, &tardis_white);
    bmp_init_container(tardis_frames[tardis_frame].black, &tardis_black);
    
    if (tardis_frames[tardis_frame].flip_x) {
      flip_bitmap_x(&tardis_white);
      flip_bitmap_x(&tardis_black);
    }

    GRect destination = layer_get_frame(me);
    destination.origin.x = 0;
    destination.origin.y = 0;
    
    // First, draw the new face.
    graphics_context_set_compositing_mode(ctx, GCompOpAssign);
    graphics_draw_bitmap_in_rect(ctx, &curr_image.bmp, destination);

    if (wipe_x > 0) {
      // Then, draw the previous face on top of it, reducing the size to wipe
      // from right to left.
      destination.size.w = wipe_x;
      graphics_draw_bitmap_in_rect(ctx, &prev_image.bmp, destination);
    }
      
    // Finally, draw the tardis on top of the wipe line.
    destination.size.w = tardis_white.bmp.bounds.size.w;
    destination.size.h = tardis_white.bmp.bounds.size.h;
    destination.origin.y = 0;
    destination.origin.x = wipe_x - destination.size.w / 2;
    graphics_context_set_compositing_mode(ctx, GCompOpOr);
    graphics_draw_bitmap_in_rect(ctx, &tardis_white.bmp, destination);
    
    destination.size.w = tardis_black.bmp.bounds.size.w;
    destination.size.h = tardis_black.bmp.bounds.size.h;
    destination.origin.y = 0;
    destination.origin.x = wipe_x - destination.size.w / 2;
    graphics_context_set_compositing_mode(ctx, GCompOpClear);
    graphics_draw_bitmap_in_rect(ctx, &tardis_black.bmp, destination);
    
    bmp_deinit_container(&tardis_white);
    bmp_deinit_container(&tardis_black);
    bmp_deinit_container(&curr_image);
    bmp_deinit_container(&prev_image);
  }
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
    prev_face_value = face_value;
    face_value = face_new;
    layer_mark_dirty(&face_layer);

    // We'll also start a transition animation.
    face_transition = true;
    transition_start = get_anim_ticks();
    set_anim_timer();
  }
}

// Triggered at 100ms intervals for transition animations.
void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  if (face_transition) {
    layer_mark_dirty(&face_layer);
    set_anim_timer();
  } else {
    clear_anim_timer();
  }
}

void handle_init(AppContextRef ctx) {
  PblTm time;

  app_ctx = ctx;
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
    },
    .timer_handler = &handle_timer,
  };
  app_event_loop(params, &handlers);
}
