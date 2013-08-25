#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"

//#define FAST_TIME 1

#define HOUR_BUZZER 1

#define MY_UUID { 0x22, 0x1E, 0xA6, 0x2F, 0xE2, 0xD0, 0x47, 0x25, 0x97, 0xC3, 0x7F, 0xB3, 0xA2, 0xAF, 0x4C, 0x0C }
PBL_APP_INFO(MY_UUID,
             "12 Doctors", "drwrose",
             1, 2, /* App version */
             RESOURCE_ID_APP_ICON,
             APP_INFO_WATCH_FACE);


#define SCREEN_WIDTH 144
#define SCREEN_HEIGHT 168

// Amount of time, in seconds, to ring the buzzer before the hour.
#define BUZZER_ANTICIPATE 2

// Number of milliseconds per animation frame
#define ANIM_TICK_MS 50

// Number of frames of animation
#define NUM_TRANSITION_FRAMES 24

AppContextRef app_ctx;
Window window;

BmpContainer mins_background;

// These are filled in only during a transition (while face_transition
// is true).
bool has_prev_image = false;
BmpContainer prev_image;
bool has_curr_image = false;
BmpContainer curr_image;

// The mask and image for the moving sprite across the wipe.
bool has_sprite_mask = false;
BmpContainer sprite_mask;
bool has_sprite = false;
BmpContainer sprite;

// The horizontal center point of the sprite.
int sprite_cx = 0;


Layer face_layer;   // The "face", in both senses (and also the hour indicator).
Layer minute_layer; // The minutes indicator.

int face_value;       // The current face on display (or transitioning into)
bool face_transition; // True if the face is in transition
bool wipe_direction;  // True for left-to-right, False for right-to-left.
bool anim_direction;  // True to reverse tardis rotation.
int transition_frame; // Frame number of current transition
int prev_face_value;  // The face we're transitioning from, or -1.
AppTimerHandle anim_timer = APP_TIMER_INVALID_HANDLE;

int minute_value;    // The current minute value displayed
int last_buzz_hour;  // The hour at which we last sounded the buzzer.

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
  int tardis;
  bool flip_x;
} TardisFrame;

#define NUM_TARDIS_FRAMES 7
TardisFrame tardis_frames[NUM_TARDIS_FRAMES] = {
  { RESOURCE_ID_TARDIS_01, false },
  { RESOURCE_ID_TARDIS_02, false },
  { RESOURCE_ID_TARDIS_03, false },
  { RESOURCE_ID_TARDIS_04, false },
  { RESOURCE_ID_TARDIS_04, true },
  { RESOURCE_ID_TARDIS_03, true },
  { RESOURCE_ID_TARDIS_02, true }
};

static const uint32_t tap_segments[] = { 50 };
VibePattern tap = {
  tap_segments,
  1,
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

#ifdef HOUR_BUZZER
int check_buzzer() {
  // Rings the buzzer if it's almost time for the hour to change.
  // Returns the amount of time in ms to wait for the next buzzer.
  time_t now = time(NULL);  

  // What hour is it right now, including the anticipate offset?
  int this_hour = (now + BUZZER_ANTICIPATE) / 3600;
  if (this_hour != last_buzz_hour) {
    if (last_buzz_hour != -1) {
      // Time to ring the buzzer.
      vibes_enqueue_custom_pattern(tap);
    }

    // Now make sure we don't ring the buzzer again for this hour.
    last_buzz_hour = this_hour;
  }

  int next_hour = this_hour + 1;
  int next_buzzer_time = next_hour * 3600 - BUZZER_ANTICIPATE;
  return (next_buzzer_time - now) * 1000;
}
#endif  // HOUR_BUZZER

// Ensures the animation/buzzer timer is running.
void set_next_timer() {
  if (anim_timer != APP_TIMER_INVALID_HANDLE) {
    app_timer_cancel_event(app_ctx, anim_timer);
    anim_timer = APP_TIMER_INVALID_HANDLE;
  }
#ifdef HOUR_BUZZER
  int next_buzzer_ms = check_buzzer();
#endif  // HOUR_BUZZER

  if (face_transition) {
    // If the animation is underway, we need to fire the timer at
    // ANIM_TICK_MS intervals.
    anim_timer = app_timer_send_event(app_ctx, ANIM_TICK_MS, 0);

  } else {
#ifdef HOUR_BUZZER
    // Otherwise, we only need a timer to tell us to buzz at (almost)
    // the top of the hour.
    anim_timer = app_timer_send_event(app_ctx, next_buzzer_ms, 0);
#endif  // HOUR_BUZZER
  }
}

void stop_transition() {
  face_transition = false;

  // Release the transition resources.
  if (has_curr_image) {
    bmp_deinit_container(&curr_image);
    has_curr_image = false;
  }

  if (has_prev_image) {
    bmp_deinit_container(&prev_image);
    has_prev_image = false;
  }

  if (has_sprite_mask) {
    bmp_deinit_container(&sprite_mask);
    has_sprite_mask = false;
  }

  if (has_sprite) {
    bmp_deinit_container(&sprite);
    has_sprite = false;
  }

  // Stop the transition timer.
  if (anim_timer != APP_TIMER_INVALID_HANDLE) {
    app_timer_cancel_event(app_ctx, anim_timer);
    anim_timer = APP_TIMER_INVALID_HANDLE;
  }
}

void start_transition(int face_new, bool force_tardis) {
  if (face_transition) {
    stop_transition();
  }

  // Update the face display.
  prev_face_value = face_value;
  face_value = face_new;

  face_transition = true;
  transition_frame = 0;

  // Initialize the transition resources.
  if (prev_face_value >= 0) {
    bmp_init_container(face_resource_ids[prev_face_value], &prev_image);
    has_prev_image = true;
  }

  bmp_init_container(face_resource_ids[face_value], &curr_image);
  has_curr_image = true;

  static const int num_sprites = 3;
  int sprite_sel;

  if (force_tardis) {
    // Force the right-to-left TARDIS transition at startup.
    wipe_direction = false;
    sprite_sel = 0;
    anim_direction = false;

  } else {
    // Choose a random transition at the top of the hour.
    wipe_direction = (rand() % 2) != 0;    // Sure, it's not 100% even, but whatever.
    sprite_sel = (rand() % num_sprites);
    anim_direction = (rand() % 2) != 0;
  }
  
  // Initialize the sprite.
  switch (sprite_sel) {
  case 0:
    // TARDIS.
    has_sprite_mask = true;
    bmp_init_container(RESOURCE_ID_TARDIS_MASK, &sprite_mask);
    
    sprite_cx = 72;
    break;

  case 1:
    // K9.
    has_sprite_mask = true;
    bmp_init_container(RESOURCE_ID_K9_MASK, &sprite_mask);
    has_sprite = true;
    bmp_init_container(RESOURCE_ID_K9, &sprite);
    sprite_cx = 41;

    if (wipe_direction) {
      flip_bitmap_x(&sprite_mask);
      flip_bitmap_x(&sprite);
      sprite_cx = sprite.bmp.bounds.size.w - sprite_cx;
    }
    break;

  case 2:
    // Dalek.
    has_sprite_mask = true;
    bmp_init_container(RESOURCE_ID_DALEK_MASK, &sprite_mask);
    has_sprite = true;
    bmp_init_container(RESOURCE_ID_DALEK, &sprite);
    sprite_cx = 74;

    if (wipe_direction) {
      flip_bitmap_x(&sprite_mask);
      flip_bitmap_x(&sprite);
      sprite_cx = sprite.bmp.bounds.size.w - sprite_cx;
    }
    break;
  }

  // Start the transition timer.
  layer_mark_dirty(&face_layer);
  set_next_timer();
}
  
void face_layer_update_callback(Layer *me, GContext* ctx) {
  int ti = 0;
  
  if (face_transition) {
    // ti ranges from 0 to NUM_TRANSITION_FRAMES over the transition.
    ti = transition_frame;
    transition_frame++;
    if (ti > NUM_TRANSITION_FRAMES) {
      stop_transition();
    }
  }

  if (!face_transition) {
    // The simple case: no transition, so just hold the current frame.
    if (face_value >= 0) {
      BmpContainer image;
      bmp_init_container(face_resource_ids[face_value], &image);
    
      GRect destination = layer_get_frame(me);
      destination.origin.x = 0;
      destination.origin.y = 0;
      
      graphics_context_set_compositing_mode(ctx, GCompOpAssign);
      graphics_draw_bitmap_in_rect(ctx, &image.bmp, destination);
      
      bmp_deinit_container(&image);
    }

  } else {
    // The complex case: we animate a transition from one face to another.

    // How far is the total animation distance from offscreen to
    // offscreen?
    int sprite_width = sprite_mask.bmp.bounds.size.w;
    int wipe_width = SCREEN_WIDTH + sprite_width;

    // Compute the current pixel position of the center of the wipe.
    // It might be offscreen on one side or the other.
    int wipe_x;
    wipe_x = wipe_width - ti * wipe_width / NUM_TRANSITION_FRAMES;
    if (wipe_direction) {
      wipe_x = wipe_width - wipe_x;
    }
    wipe_x = wipe_x - (sprite_width - sprite_cx);

    GRect destination = layer_get_frame(me);
    destination.origin.x = 0;
    destination.origin.y = 0;
    
    if (wipe_direction) {
      // First, draw the previous face.
      if (wipe_x < SCREEN_WIDTH) {
        if (has_prev_image) {
          graphics_context_set_compositing_mode(ctx, GCompOpAssign);
          graphics_draw_bitmap_in_rect(ctx, &prev_image.bmp, destination);
        } else {
          graphics_context_set_fill_color(ctx, GColorBlack);
          graphics_fill_rect(ctx, destination, 0, GCornerNone);
        }
      }
      
      if (wipe_x > 0) {
        // Then, draw the new face on top of it, reducing the size to wipe
        // from right to left.
        if (has_curr_image) {
          destination.size.w = wipe_x;
          graphics_draw_bitmap_in_rect(ctx, &curr_image.bmp, destination);
        }
      }
    } else {
      // First, draw the new face.
      if (wipe_x < SCREEN_WIDTH) {
        if (has_curr_image) {
          graphics_context_set_compositing_mode(ctx, GCompOpAssign);
          graphics_draw_bitmap_in_rect(ctx, &curr_image.bmp, destination);
        }
      }
      
      if (wipe_x > 0) {
        // Then, draw the previous face on top of it, reducing the size to wipe
        // from right to left.
        destination.size.w = wipe_x;
        if (has_prev_image) {
          graphics_draw_bitmap_in_rect(ctx, &prev_image.bmp, destination);
        } else {
          graphics_context_set_fill_color(ctx, GColorBlack);
          graphics_fill_rect(ctx, destination, 0, GCornerNone);
        }
      }
    }

    if (has_sprite_mask) {
      // Then, draw the sprite on top of the wipe line.
      destination.size.w = sprite_mask.bmp.bounds.size.w;
      destination.size.h = sprite_mask.bmp.bounds.size.h;
      destination.origin.y = (SCREEN_HEIGHT - destination.size.h) / 2;
      destination.origin.x = wipe_x - sprite_cx;
      graphics_context_set_compositing_mode(ctx, GCompOpClear);
      graphics_draw_bitmap_in_rect(ctx, &sprite_mask.bmp, destination);
      
      if (has_sprite) {
        // Fixed sprite case.
        graphics_context_set_compositing_mode(ctx, GCompOpOr);
        graphics_draw_bitmap_in_rect(ctx, &sprite.bmp, destination);
      } else {
        // Tardis case.  Since it's animated, but we don't have enough
        // RAM to hold all the frames at once, we have to load one frame
        // at a time as we need it.
        BmpContainer tardis;
        int af = ti % NUM_TARDIS_FRAMES;
        if (anim_direction) {
          af = (NUM_TARDIS_FRAMES - 1) - af;
        }
        bmp_init_container(tardis_frames[af].tardis, &tardis);
        if (tardis_frames[af].flip_x) {
          flip_bitmap_x(&tardis);
        }
        
        graphics_context_set_compositing_mode(ctx, GCompOpOr);
        graphics_draw_bitmap_in_rect(ctx, &tardis.bmp, destination);
        
        bmp_deinit_container(&tardis);
      }
      
      // Finally, re-draw the minutes background card on top of the sprite.
      destination.size.w = mins_background.bmp.bounds.size.w;
      destination.size.h = mins_background.bmp.bounds.size.h;
      destination.origin.x = SCREEN_WIDTH - destination.size.w;
      destination.origin.y = SCREEN_HEIGHT - destination.size.h;
      graphics_context_set_compositing_mode(ctx, GCompOpOr);
      graphics_draw_bitmap_in_rect(ctx, &mins_background.bmp, destination);
    }
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
  if (face_value == -1) {
    // We haven't loaded yet.
    return;
  }

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

  if (face_new != face_value && !face_transition) {
    start_transition(face_new, false);
  }

  set_next_timer();
}

// Triggered at ANIM_TICK_MS intervals for transition animations; also
// triggered occasionally to check the hour buzzer.
void handle_timer(AppContextRef ctx, AppTimerHandle handle, uint32_t cookie) {
  if (face_transition) {
    layer_mark_dirty(&face_layer);
  }

  set_next_timer();
}

void handle_init(AppContextRef ctx) {
  PblTm pbltime;

  srand(time(NULL));
  resource_init_current_app(&APP_RESOURCES);

  face_transition = false;
  get_time(&pbltime);
  face_value = -1;
  last_buzz_hour = -1;
  minute_value = pbltime.tm_min;
  
  app_ctx = ctx;
  window_init(&window, "12 Doctors");
  window_stack_push(&window, false /* not animated */);

  bmp_init_container(RESOURCE_ID_MINS_BACKGROUND, &mins_background);

  layer_init(&face_layer, window.layer.frame);
  face_layer.update_proc = &face_layer_update_callback;
  layer_add_child(&window.layer, &face_layer);

  layer_init(&minute_layer, GRect(95, 134, 54, 35));
  minute_layer.update_proc = &minute_layer_update_callback;
  layer_add_child(&window.layer, &minute_layer);

  start_transition(pbltime.tm_hour % 12, true);
}

void handle_deinit(AppContextRef ctx) {
  (void)ctx;

  bmp_deinit_container(&mins_background);
  stop_transition();
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
