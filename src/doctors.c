#include <pebble.h>

#include "doctors.h"
#include "assert.h"
#include "bluetooth_indicator.h"
#include "battery_gauge.h"
#include "config_options.h"
#include "bwd.h"
#include "lang_table.h"
#include "qapp_log.h"
#include "../resources/lang_table.c"
#include "../resources/generated_config.h"
#include "../resources/generated_config.c"

// frame placements based on screen shape.
#if defined(PBL_PLATFORM_EMERY)
// Emery 200x228

GRect mm_layer_box = { { 159, 182 }, { 68, 48 } };
GRect mins_background_box = { { 0, 4 }, { 68, 42 } };
GRect mins_mm_text_box = { { 1, 0 }, { 81, 58 } };

GRect hhmm_layer_box = { { 86, 182 }, { 114, 48 } };
GRect hours_background_box = { { 0, 4 }, { 114, 42 } };
GRect hours_text_box = { { -20, 0 }, { 68, 48 } };
GRect mins_hhmm_text_box = { { 48, 0 }, { 132, 48 } };

GRect date_layer_box = { { 0, 194 }, { 68, 34 } };
GRect date_background_box = { { 0, 0 }, { 68, 34 } };
GRect date_text_box = { { 0, 0 }, { 68, 34 } };

#elif defined(PBL_ROUND)
// Round 180x180 (Chalk)

GRect mm_layer_box = { { 44, 151 }, { 92, 29 } };
GRect mins_background_box = { { 0, 0 }, { 92, 29 } };
GRect mins_mm_text_box = { { 22, -6 }, { 60, 35 } };

GRect hhmm_layer_box = { { 30, 146 }, { 120, 34 } };
GRect hours_background_box = { { 0, 0 }, { 120, 34 } };
GRect hours_text_box = { { 3, -5 }, { 50, 35 } };
GRect mins_hhmm_text_box = { { 52, -5 }, { 97, 35 } };

GRect date_layer_box = { { 49, 0 }, { 82, 22 } };
GRect date_background_box = { { 0, 0 }, { 82, 22 } };
GRect date_text_box = { { 16, -2 }, { 50, 25 } };

#else
// Rect 144x168 (Aplite, Basalt, Diorite)

GRect mm_layer_box = { { 94, 134 }, { 50, 35 } };
GRect mins_background_box = { { 0, 3 }, { 50, 31 } };
GRect mins_mm_text_box = { { 1, 0 }, { 60, 35 } };

GRect hhmm_layer_box = { { 60, 134 }, { 84, 35 } };
GRect hours_background_box = { { 0, 3 }, { 84, 31 } };
GRect hours_text_box = { { -15, 0 }, { 50, 35 } };
GRect mins_hhmm_text_box = { { 35, 0 }, { 97, 35 } };

GRect date_layer_box = { { 0, 143 }, { 50, 25 } };
GRect date_background_box = { { 0, 0 }, { 50, 25 } };
GRect date_text_box = { { 0, 0 }, { 50, 25 } };

#endif  // shape

// The frequency throughout the day at which the buzzer sounds, in seconds.
#define BUZZER_FREQ 3600

// Amount of time, in seconds, to ring the buzzer before the hour.
#define BUZZER_ANTICIPATE 2

// Number of milliseconds per animation frame
#define ANIM_TICK_MS 50

// Number of frames of animation
#define NUM_TRANSITION_FRAMES_HOUR 24
#define NUM_TRANSITION_FRAMES_STARTUP 10

Window *window;

BitmapWithData mins_background;
BitmapWithData hours_background;
BitmapWithData date_background;

// The horizontal center point of the sprite.
int sprite_cx = 0;
bool any_obstructed_area = false;


Layer *face_layer;   // The "face", in both senses (and also the hour indicator).
Layer *mm_layer;   // The time indicator when minutes only are displayed.
Layer *hhmm_layer; // The time indicator when hours and minutes are displayed.
Layer *date_layer; // optional day/date display.

int face_value;       // The current face on display (or transitioning into)

// The current face bitmap, each slice:
typedef struct {
  int face_value;
  BitmapWithData face_image;
} VisibleFace;
VisibleFace visible_face[NUM_SLICES];

#ifndef PBL_BW
// A table of alternate color schemes for the Dalek bitmap (color
// builds only, of course).
typedef struct {
  uint8_t cb_argb8, c1_argb8, c2_argb8, c3_argb8;
} ColorMap;

ColorMap dalek_colors[] = {
  { GColorBlackARGB8, GColorWhiteARGB8, GColorLightGrayARGB8, GColorRedARGB8, },
  { GColorBlackARGB8, GColorWhiteARGB8, GColorLightGrayARGB8, GColorOrangeARGB8, },
  { GColorBlackARGB8, GColorWhiteARGB8, GColorDarkGrayARGB8, GColorWhiteARGB8, },
  { GColorBlackARGB8, GColorWhiteARGB8, GColorYellowARGB8, GColorWhiteARGB8, },
  { GColorBlackARGB8, GColorWhiteARGB8, GColorLightGrayARGB8, GColorBlueARGB8, },
  { GColorBlackARGB8, GColorWhiteARGB8, GColorLightGrayARGB8, GColorYellowARGB8, },
  { GColorBlackARGB8, GColorWhiteARGB8, GColorLightGrayARGB8, GColorJaegerGreenARGB8, },
  { GColorBlackARGB8, GColorWhiteARGB8, GColorVeryLightBlueARGB8, GColorBabyBlueEyesARGB8, },
};
#define NUM_DALEK_COLORS (sizeof(dalek_colors) / sizeof(ColorMap))

#endif // PBL_BW

// The transitioning face slice.
int next_face_value;
int next_face_slice;
BitmapWithData next_face_image;


bool face_transition; // True if the face is in transition
bool wipe_direction;  // True for left-to-right, False for right-to-left.
bool anim_direction;  // True to reverse tardis rotation.
int transition_frame; // Frame number of current transition
int num_transition_frames;  // Total frames for transition

// The mask and image for the moving sprite across the wipe.
int sprite_sel;
BitmapWithData sprite_mask;
BitmapWithData sprite;
int sprite_width;

// Triggered at ANIM_TICK_MS intervals for transition animations; also
// triggered occasionally to check the hour buzzer.
AppTimer *anim_timer = NULL;

// Triggered at 500 ms intervals to blink the colon.
AppTimer *blink_timer = NULL;

int hour_value;      // The decimal hour value displayed (if enabled).
int minute_value;    // The current minute value displayed
int second_value;    // The current second value displayed.  Actually we only blink the colon, rather than actually display a value, but whatever.
bool hide_colon;     // Set true every half-second to blink the colon off.
int last_buzz_hour;  // The hour at which we last sounded the buzzer.
int day_value;       // The current day-of-the-week displayed (if enabled).
int date_value;      // The current date-of-the-week displayed (if enabled).

#define SPRITE_TARDIS 0
#define SPRITE_K9     1
#define SPRITE_DALEK  2
#define NUM_SPRITES   3

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

static const uint32_t tap_segments[] = { 75, 100, 75 };
VibePattern tap = {
  tap_segments,
  3,
};

// Reverse the bits of a byte.
// http://www.graphics.stanford.edu/~seander/bithacks.html#BitReverseTable
uint8_t reverse_bits(uint8_t b) {
  return ((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
}

// Reverse the high nibble and low nibble of a byte.
uint8_t reverse_nibbles(uint8_t b) {
  return ((b & 0xf) << 4) | ((b >> 4) & 0xf);
}

// Horizontally flips the indicated GBitmap in-place.  Requires
// that the width be a multiple of 8 pixels.
void flip_bitmap_x(GBitmap *image) {
  if (image == NULL) {
    // Trivial no-op.
    return;
  }

  int height = gbitmap_get_bounds(image).size.h;
  int width = gbitmap_get_bounds(image).size.w;
  int pixels_per_byte = 8;

#ifndef PBL_BW
  switch (gbitmap_get_format(image)) {
  case GBitmapFormat1Bit:
  case GBitmapFormat1BitPalette:
    pixels_per_byte = 8;
    break;

  case GBitmapFormat2BitPalette:
    pixels_per_byte = 4;
    break;

  case GBitmapFormat4BitPalette:
    pixels_per_byte = 2;
    break;

  case GBitmapFormat8Bit:
  case GBitmapFormat8BitCircular:
    pixels_per_byte = 1;
    break;
  }
#endif  // PBL_BW

  assert(width % pixels_per_byte == 0);  // This must be an even divisor, by our convention.
  int width_bytes = width / pixels_per_byte;
  int stride = gbitmap_get_bytes_per_row(image);
  assert(stride >= width_bytes);

  //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "flip_bitmap_x, width_bytes = %d, stride=%d, format=%d", width_bytes, stride, gbitmap_get_format(image));

  uint8_t *data = gbitmap_get_data(image);

  for (int y = 0; y < height; ++y) {
    uint8_t *row = data + y * stride;
    switch (pixels_per_byte) {
    case 8:
      for (int x1 = (width_bytes - 1) / 2; x1 >= 0; --x1) {
        int x2 = width_bytes - 1 - x1;
        uint8_t b = reverse_bits(row[x1]);
        row[x1] = reverse_bits(row[x2]);
        row[x2] = b;
      }
      break;

#ifndef PBL_BW
    case 4:
      // TODO.
      break;

    case 2:
      for (int x1 = (width_bytes - 1) / 2; x1 >= 0; --x1) {
        int x2 = width_bytes - 1 - x1;
        uint8_t b = reverse_nibbles(row[x1]);
        row[x1] = reverse_nibbles(row[x2]);
        row[x2] = b;
      }
      break;

    case 1:
      for (int x1 = (width_bytes - 1) / 2; x1 >= 0; --x1) {
        int x2 = width_bytes - 1 - x1;
        uint8_t b = row[x1];
        row[x1] = row[x2];
        row[x2] = b;
      }
      break;
#endif  // PBL_BW
    }
  }
}

int check_buzzer() {
  // Rings the buzzer if it's almost time for the hour to change.
  // Returns the amount of time in ms to wait for the next buzzer.
  time_t now = time(NULL);

  // What hour is it right now, including the anticipate offset?
  int this_hour = (now + BUZZER_ANTICIPATE) / BUZZER_FREQ;
  if (this_hour != last_buzz_hour) {
    if (last_buzz_hour != -1) {
      // Time to ring the buzzer.
      if (config.hour_buzzer) {
        vibes_enqueue_custom_pattern(tap);
        //vibes_double_pulse();
      }
    }

    // Now make sure we don't ring the buzzer again for this hour.
    last_buzz_hour = this_hour;
  }

  int next_hour = this_hour + 1;
  int next_buzzer_time = next_hour * BUZZER_FREQ - BUZZER_ANTICIPATE;

  return (next_buzzer_time - now) * 1000;
}

void set_next_timer();

// Triggered at ANIM_TICK_MS intervals for transition animations; also
// triggered occasionally to check the hour buzzer.
void handle_timer(void *data) {
  anim_timer = NULL;  // When the timer is handled, it is implicitly canceled.

  if (face_transition) {
    layer_mark_dirty(face_layer);
  }

  set_next_timer();
}

// Triggered at 500 ms intervals to blink the colon.
void handle_blink(void *data) {
  blink_timer = NULL;  // When the timer is handled, it is implicitly canceled.

  if (config.second_hand) {
    hide_colon = true;
    layer_mark_dirty(mm_layer);
    layer_mark_dirty(hhmm_layer);
  }

  if (blink_timer != NULL) {
    app_timer_cancel(blink_timer);
    blink_timer = NULL;
  }
}

// Ensures the animation/buzzer timer is running.
void set_next_timer() {
  if (anim_timer != NULL) {
    app_timer_cancel(anim_timer);
    anim_timer = NULL;
  }
  int next_buzzer_ms = check_buzzer();

  if (face_transition) {
    // If the animation is underway, we need to fire the timer at
    // ANIM_TICK_MS intervals.
    anim_timer = app_timer_register(ANIM_TICK_MS, &handle_timer, 0);

  } else {
    // Otherwise, we only need a timer to tell us to buzz at (almost)
    // the top of the hour.
    anim_timer = app_timer_register(next_buzzer_ms, &handle_timer, 0);
  }
}

void stop_transition() {
  face_transition = false;

  // Release the transition resources.
  next_face_value = -1;
  next_face_slice = -1;
  bwd_destroy(&next_face_image);
  bwd_destroy(&sprite_mask);
  bwd_destroy(&sprite);

  // Stop the transition timer.
  if (anim_timer != NULL) {
    app_timer_cancel(anim_timer);
    anim_timer = NULL;
  }
  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "stop_transition(), memory used, free is %d, %d", heap_bytes_used(), heap_bytes_free());
}

void start_transition(int face_new, bool for_startup) {
  if (face_transition) {
    stop_transition();
  }
  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "start_transition(%d, %d)", face_new, for_startup);

  // Update the face display.
  face_value = face_new;

  face_transition = true;
  transition_frame = 0;
  num_transition_frames = NUM_TRANSITION_FRAMES_HOUR;

  if (for_startup) {
    // Force the right-to-left TARDIS transition at startup.
    wipe_direction = false;
    sprite_sel = SPRITE_TARDIS;
    anim_direction = false;

    // We used to want this to go super-fast at startup, to match the
    // speed of the system wipe, but we no longer try to do this
    // (since the system wipe is different nowadays anyway).
    //num_transition_frames = NUM_TRANSITION_FRAMES_STARTUP;

  } else {
    // Choose a random transition at the top of the hour.

    wipe_direction = (rand() % 2) != 0;    // Sure, it's not 100% even, but whatever.
    sprite_sel = (rand() % NUM_SPRITES);
    anim_direction = (rand() % 2) != 0;
  }

  // Initialize the sprite.
  switch (sprite_sel) {
  case SPRITE_TARDIS:
    //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "starting transition TARDIS, memory used, free is %d, %d", heap_bytes_used(), heap_bytes_free());
#ifdef PBL_BW
    sprite_mask = rle_bwd_create(RESOURCE_ID_TARDIS_MASK);
#endif  // PBL_BW
    //sprite_width = gbitmap_get_bounds(sprite_mask.bitmap).size.w;
    sprite_width = 112;
    sprite_cx = 72;
    break;

  case SPRITE_K9:
    //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "starting transition K9, memory used, free is %d, %d", heap_bytes_used(), heap_bytes_free());
#ifdef PBL_BW
    sprite_mask = rle_bwd_create(RESOURCE_ID_K9_MASK);
#endif  // PBL_BW
    sprite = rle_bwd_create(RESOURCE_ID_K9);
    if (sprite.bitmap != NULL) {
      sprite_width = gbitmap_get_bounds(sprite.bitmap).size.w;
      //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "k9 loaded %p, format = %d", sprite.bitmap, gbitmap_get_format(sprite.bitmap));
    }
    sprite_cx = 41;

    if (wipe_direction) {
      flip_bitmap_x(sprite_mask.bitmap);
      flip_bitmap_x(sprite.bitmap);
      sprite_cx = sprite_width - sprite_cx;
    }
    break;

  case SPRITE_DALEK:
    //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "starting transition DALEK, memory used, free is %d, %d", heap_bytes_used(), heap_bytes_free());
#ifdef PBL_BW
    sprite_mask = rle_bwd_create(RESOURCE_ID_DALEK_MASK);
#endif  // PBL_BW
    sprite = rle_bwd_create(RESOURCE_ID_DALEK);
    if (sprite.bitmap != NULL) {
      sprite_width = gbitmap_get_bounds(sprite.bitmap).size.w;
      //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "dalek loaded %p, format = %d", sprite.bitmap, gbitmap_get_format(sprite.bitmap));
#ifndef PBL_BW
      // Pick a random color scheme for the Dalek.
      int color_sel = (rand() % NUM_DALEK_COLORS);
      ColorMap *cm = &dalek_colors[color_sel];
      bwd_remap_colors(&sprite, (GColor8){.argb=cm->cb_argb8}, (GColor8){.argb=cm->c1_argb8}, (GColor8){.argb=cm->c2_argb8}, (GColor8){.argb=cm->c3_argb8}, false);
#endif  // PBL_BW
    }
    sprite_cx = 74;

    if (wipe_direction) {
      flip_bitmap_x(sprite_mask.bitmap);
      flip_bitmap_x(sprite.bitmap);
      sprite_cx = sprite_width - sprite_cx;
    }
    break;
  }

  // Start the transition timer.
  layer_mark_dirty(face_layer);
  set_next_timer();
}

// Loads next_face_image with the si'th slice of face_value.  No-op if
// it's already loaded.
void
load_next_face_slice(int si, int face_value) {
  if (next_face_value != face_value || next_face_slice != si) {
    bwd_destroy(&next_face_image);
    next_face_value = face_value;
    next_face_slice = si;
    int resource_id = face_resource_ids[face_value][si];
    next_face_image = rle_bwd_create(resource_id);
#ifdef PBL_COLOR
    assert(next_face_image.bitmap == NULL || gbitmap_get_format(next_face_image.bitmap) == GBitmapFormat4BitPalette || gbitmap_get_format(next_face_image.bitmap) == GBitmapFormat2BitPalette);
#endif  // PBL_COLOR
  }
}

// Ensures the bitmap for face_value is loaded for slice si.  No-op if
// it's already loaded.
void
load_face_slice(int si, int face_value) {
  if (visible_face[si].face_value != face_value) {
    bwd_destroy(&(visible_face[si].face_image));
    visible_face[si].face_value = face_value;
    int resource_id = face_resource_ids[face_value][si];
    visible_face[si].face_image = rle_bwd_create(resource_id);
  }
}

#if NUM_SLICES == 1

// The following code is the single-slice version of face-drawing,
// assuming that face bitmaps are complete and full-screen.

// Draw the face in transition from one to the other, with the
// division line at wipe_x.

// Draws the indicated fullscreen bitmap only to the left of wipe_x.
void draw_fullscreen_wiped(GContext *ctx, BitmapWithData image, int wipe_x) {
  if (wipe_x <= 0) {
    // Trivial no-op.
    return;
  }

  if (wipe_x > SCREEN_WIDTH) {
    // Trivial fill.
    wipe_x = SCREEN_WIDTH;
  }

  GRect destination;
  destination.origin.x = 0;
  destination.origin.y = 0;
  destination.size.w = wipe_x;
  destination.size.h = SCREEN_HEIGHT;

  if (image.bitmap == NULL) {
    // The bitmap wasn't loaded successfully; just clear the region.
    // This is a fallback.
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, destination, 0, GCornerNone);
  } else {

    // Draw the bitmap in the region.
#ifdef PBL_ROUND
    // The round model, complicated because we have created a concept
    // of a GBitmapFormat4BitPaletteCircular: it's a 180x180 4-bit
    // palette image, with only a subset of pixels included that are
    // within the visible circle.  It's exactly the same subset
    // described in the GBitmapFormat8BitCircular format (used by the
    // framebuffer), but we have to do the drawing operations
    // ourselves.
    uint8_t *source_data = gbitmap_get_data(image.bitmap);
    GColor *source_palette = gbitmap_get_palette(image.bitmap);

    int num_bits;
    switch (gbitmap_get_format(image.bitmap)) {
    case GBitmapFormat4BitPalette:
      num_bits = 4;
      break;
    case GBitmapFormat2BitPalette:
      num_bits = 2;
      break;
    default:
      assert(false);
      num_bits = 1;
    }
    int bit_mask = (1 << num_bits) - 1;
    int pixels_per_byte = 8 / num_bits;

    uint8_t *sp = source_data;
    int bit_shift = 8 - num_bits;

    GBitmap *fb = graphics_capture_frame_buffer(ctx);

    for (int y = 0; y < SCREEN_HEIGHT; ++y) {
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);
      uint8_t *row = &info.data[info.min_x];
      uint8_t *dp = row;
      int stop_x = info.max_x < wipe_x ? info.max_x : wipe_x;
      if (stop_x < info.min_x) {
        stop_x = info.min_x - 1;
      }
      for (int x = info.min_x; x <= stop_x; ++x) {
        int value = ((*sp) >> bit_shift) & bit_mask;

        *dp = source_palette[value].argb;
        ++dp;

        bit_shift -= num_bits;
        if (bit_shift < 0) {
          bit_shift = 8 - num_bits;
          ++sp;
        }
      }

      if (stop_x < info.max_x) {
        int skip_pixels = info.max_x - stop_x;
        // Here we are at the end of the row; skip skip_pixels of the
        // source.
        sp += skip_pixels / pixels_per_byte;
        int additional_pixels = skip_pixels % pixels_per_byte;
        while (additional_pixels > 0) {
          bit_shift -= num_bits;
          if (bit_shift < 0) {
            bit_shift = 8 - num_bits;
            ++sp;
          }
          --additional_pixels;
        }
      }
    }

    graphics_release_frame_buffer(ctx, fb);

#else  // PBL_ROUND
    // The rectangular model, pretty straightforward.
    graphics_draw_bitmap_in_rect(ctx, image.bitmap, destination);
#endif  // PBL_ROUND
  }
}

// Draws the indicated fullscreen bitmap completely.
void draw_fullscreen_complete(GContext *ctx, BitmapWithData image) {
  draw_fullscreen_wiped(ctx, image, SCREEN_WIDTH);
}

void draw_face_transition(Layer *me, GContext *ctx, int wipe_x) {
  if (wipe_direction) {
    // Wiping left-to-right.

    // Draw the old face, and draw the visible portion of the new face
    // on top of it.
    draw_fullscreen_complete(ctx, visible_face[0].face_image);
    load_next_face_slice(0, face_value);
    draw_fullscreen_wiped(ctx, next_face_image, wipe_x);

  } else {
    // Wiping right-to-left.

    // Draw the new face, and then draw the visible portion of the old
    // face on top of it.
    load_next_face_slice(0, face_value);
    draw_fullscreen_complete(ctx, next_face_image);
    draw_fullscreen_wiped(ctx, visible_face[0].face_image, wipe_x);
  }
}

void draw_face_complete(Layer *me, GContext *ctx) {
  load_face_slice(0, face_value);
  draw_fullscreen_complete(ctx, visible_face[0].face_image);
}

#else  // NUM_SLICES

// The following code is the multi-slice version of face-drawing,
// which is complicated because we have pre-sliced all of the face
// bitmaps into NUM_SLICES vertical slices, so we can load and draw
// these slices one at a time.  During a transition, we display n
// slices of the old face, and NUM_SLICES - n - 1 of the new face,
// with a single slice that might have parts of both faces visible at
// once.  So we only have to double up on that one transitioning
// slice, which helps the RAM utilization a great deal.

// In practice, this causes visible timing hiccups as we load each
// slice during the animation, and it's not actually necessary as the
// 4-bit palette versions of these bitmaps do fit entirely in memory.
// So this is no longer used.

// Draws the indicated face_value for slice si.
void
draw_face_slice(Layer *me, GContext *ctx, int si) {
  GRect destination = layer_get_frame(me);
  destination.origin.x = slice_points[si];
  destination.origin.y = 0;
  destination.size.w = slice_points[si + 1] - slice_points[si];

  if (visible_face[si].face_image.bitmap == NULL) {
    // The bitmap wasn't loaded successfully; just clear the
    // region.  This is a fallback.
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, destination, 0, GCornerNone);
  } else {
    // The bitmap was loaded successfully, so draw it.
    graphics_draw_bitmap_in_rect(ctx, visible_face[si].face_image.bitmap, destination);
  }
}

// Draw the face in transition from one to the other, with the
// division line at wipe_x.

void draw_face_transition(Layer *me, GContext *ctx, int wipe_x) {
  // The slice number within which the transition is currently
  // happening.
  int wipe_slice = (wipe_x * NUM_SLICES) / SCREEN_WIDTH;
  if (wipe_slice < 0) {
    wipe_slice = 0;
  } else if (wipe_slice >= NUM_SLICES) {
    wipe_slice = NUM_SLICES - 1;
  }

  GRect destination;
  destination.origin.x = slice_points[wipe_slice];
  destination.origin.y = 0;
  destination.size.w = slice_points[wipe_slice + 1] - slice_points[wipe_slice];
  destination.size.h = SCREEN_HEIGHT;

  if (wipe_direction) {
    // Wiping left-to-right.

    // Draw the old face within the wipe_slice, and draw the visible
    // portion of the new face on top of it.
    draw_face_slice(me, ctx, wipe_slice);
    if (wipe_x > destination.origin.x) {
      load_next_face_slice(wipe_slice, face_value);
      destination.size.w = wipe_x - destination.origin.x;
      if (next_face_image.bitmap == NULL) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, destination, 0, GCornerNone);
      } else {
        graphics_draw_bitmap_in_rect(ctx, next_face_image.bitmap, destination);
      }
    }

    // Draw all of the slices left of the wipe_slice in the new
    // face.
    for (int si = 0; si < wipe_slice; ++si) {
      load_face_slice(si, face_value);
      draw_face_slice(me, ctx, si);
    }

    // Draw all of the slices right of the wipe_slice
    // in the old face.
    for (int si = wipe_slice + 1; si < NUM_SLICES; ++si) {
      draw_face_slice(me, ctx, si);
    }

  } else {
    // Wiping right-to-left.

    // Draw the new face within the wipe_slice, and then draw
    // the visible portion of the old face on top of it.
    load_next_face_slice(wipe_slice, face_value);
    if (next_face_image.bitmap == NULL) {
      graphics_context_set_fill_color(ctx, GColorBlack);
      graphics_fill_rect(ctx, destination, 0, GCornerNone);
    } else {
      graphics_draw_bitmap_in_rect(ctx, next_face_image.bitmap, destination);
    }
    if (wipe_x > destination.origin.x) {
      destination.size.w = wipe_x - destination.origin.x;
      if (visible_face[wipe_slice].face_image.bitmap == NULL) {
        graphics_context_set_fill_color(ctx, GColorBlack);
        graphics_fill_rect(ctx, destination, 0, GCornerNone);
      } else {
        graphics_draw_bitmap_in_rect(ctx, visible_face[wipe_slice].face_image.bitmap, destination);
      }
    }

    // Draw all of the slices left of the wipe_slice in the old
    // face.
    for (int si = 0; si < wipe_slice; ++si) {
      draw_face_slice(me, ctx, si);
    }

    // Draw all of the slices right of the wipe_slice
    // in the new face.
    for (int si = wipe_slice + 1; si < NUM_SLICES; ++si) {
      load_face_slice(si, face_value);
      draw_face_slice(me, ctx, si);
    }
  }
}

void draw_face_complete(Layer *me, GContext *ctx) {
  for (int si = 0; si < NUM_SLICES; ++si) {
    load_face_slice(si, face_value);
    draw_face_slice(me, ctx, si);
  }
}

#endif  // NUM_SLICES

void root_layer_update_callback(Layer *me, GContext *ctx) {
  //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "root_layer");

  // Only bother filling in the root layer if part of the window is
  // obstructed.  We do this to ensure the entire window is cleared in
  // case we're not drawing all of it.
  if (any_obstructed_area) {
    GRect destination = layer_get_frame(me);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, destination, 0, GCornerNone);
  }
}

void face_layer_update_callback(Layer *me, GContext *ctx) {
  //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "face_layer");
  int ti = 0;

  if (face_transition) {
    // ti ranges from 0 to num_transition_frames over the transition.
    ti = transition_frame;
    transition_frame++;
    if (ti > num_transition_frames) {
      stop_transition();
    }
  }

  if (!face_transition) {
    // The simple case: no transition, so just hold the current frame.
    if (face_value >= 0) {
      graphics_context_set_compositing_mode(ctx, GCompOpAssign);

      // This means we draw all of the slices with the same face;
      // simple as can be.
      draw_face_complete(me, ctx);
    }

  } else {
    // The complex case: we animate a transition from one face to another.

    // How far is the total animation distance from offscreen to
    // offscreen?
    int wipe_width = SCREEN_WIDTH + sprite_width;

    // Compute the current pixel position of the center of the wipe.
    // It might be offscreen on one side or the other.
    int wipe_x;
    wipe_x = wipe_width - ti * wipe_width / num_transition_frames;
    if (wipe_direction) {
      wipe_x = wipe_width - wipe_x;
    }
    wipe_x = wipe_x - (sprite_width - sprite_cx);
    draw_face_transition(me, ctx, wipe_x);

    if (sprite_mask.bitmap != NULL) {
      // Then, draw the sprite on top of the wipe line.
      GRect destination;
      destination.size.w = gbitmap_get_bounds(sprite_mask.bitmap).size.w;
      destination.size.h = gbitmap_get_bounds(sprite_mask.bitmap).size.h;
      destination.origin.y = (SCREEN_HEIGHT - destination.size.h) / 2;
      destination.origin.x = wipe_x - sprite_cx;
      graphics_context_set_compositing_mode(ctx, GCompOpClear);
      graphics_draw_bitmap_in_rect(ctx, sprite_mask.bitmap, destination);
    }

    if (sprite.bitmap != NULL) {
      // Fixed sprite case.
      GRect destination;
      destination.size.w = gbitmap_get_bounds(sprite.bitmap).size.w;
      destination.size.h = gbitmap_get_bounds(sprite.bitmap).size.h;
      destination.origin.y = (SCREEN_HEIGHT - destination.size.h) / 2;
      destination.origin.x = wipe_x - sprite_cx;
#ifdef PBL_BW
      graphics_context_set_compositing_mode(ctx, GCompOpOr);
#else  //  PBL_BW
      graphics_context_set_compositing_mode(ctx, GCompOpSet);
#endif //  PBL_BW
      graphics_draw_bitmap_in_rect(ctx, sprite.bitmap, destination);

    } else if (sprite_sel == SPRITE_TARDIS) {
      // Tardis case.  Since it's animated, but we don't have enough
      // RAM to hold all the frames at once, we have to load one
      // frame at a time as we need it.
      int af = ti % NUM_TARDIS_FRAMES;
      if (anim_direction) {
        af = (NUM_TARDIS_FRAMES - 1) - af;
      }
      BitmapWithData tardis = png_bwd_create(tardis_frames[af].tardis);
      if (tardis.bitmap != NULL) {
        if (tardis_frames[af].flip_x) {
          flip_bitmap_x(tardis.bitmap);
        }
        //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "tardis loaded %p, format = %d", sprite.bitmap, gbitmap_get_format(tardis.bitmap));

        GRect destination;
        destination.size.w = gbitmap_get_bounds(tardis.bitmap).size.w;
        destination.size.h = gbitmap_get_bounds(tardis.bitmap).size.h;
        destination.origin.y = (SCREEN_HEIGHT - destination.size.h) / 2;
        destination.origin.x = wipe_x - sprite_cx;

#ifdef PBL_BW
        graphics_context_set_compositing_mode(ctx, GCompOpOr);
#else  //  PBL_BW
        graphics_context_set_compositing_mode(ctx, GCompOpSet);
#endif //  PBL_BW
        graphics_draw_bitmap_in_rect(ctx, tardis.bitmap, destination);

        bwd_destroy(&tardis);
      }
    }
  }
}

void mm_layer_update_callback(Layer *me, GContext* ctx) {
  //  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "mm_layer");
  GFont font;
  static const int buffer_size = 128;
  char buffer[buffer_size];

  // This layer includes only the minutes digits (preceded by a colon).

  graphics_context_set_compositing_mode(ctx, GCompOpOr);

  // Draw the background card for the minutes digits.
  if (mins_background.bitmap == NULL) {
    mins_background = rle_bwd_create(RESOURCE_ID_MINS_BACKGROUND);
    //app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "mins_background loaded %p, format = %d", mins_background.bitmap, gbitmap_get_format(mins_background.bitmap));
  }
  if (mins_background.bitmap != NULL) {
    graphics_draw_bitmap_in_rect(ctx, mins_background.bitmap, mins_background_box);
  }

  font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);

#ifdef PBL_COLOR
  graphics_context_set_text_color(ctx, GColorDukeBlue);
#else  // PBL_COLOR
  graphics_context_set_text_color(ctx, GColorBlack);
#endif // PBL_COLOR

  // Draw the (possibly blinking) colon.
  if (!config.second_hand || !hide_colon) {
    graphics_draw_text(ctx, ":", font, mins_mm_text_box,
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
                       NULL);
  }

  // draw minutes
  snprintf(buffer, buffer_size, " %02d", minute_value);
  graphics_draw_text(ctx, buffer, font, mins_mm_text_box,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
                     NULL);
}

void hhmm_layer_update_callback(Layer *me, GContext* ctx) {
  //  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "hhmm_layer");
  GFont font;
  static const int buffer_size = 128;
  char buffer[buffer_size];

  // This layer includes both the hours and the minutes digits (with a
  // colon).

  graphics_context_set_compositing_mode(ctx, GCompOpOr);

  // Draw the background card for the hours digits.
  if (hours_background.bitmap == NULL) {
    hours_background = rle_bwd_create(RESOURCE_ID_HOURS_BACKGROUND);
  }
  if (hours_background.bitmap != NULL) {
    graphics_draw_bitmap_in_rect(ctx, hours_background.bitmap, hours_background_box);
  }

  font = fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK);

#ifdef PBL_COLOR
  graphics_context_set_text_color(ctx, GColorDukeBlue);
#else  // PBL_COLOR
  graphics_context_set_text_color(ctx, GColorBlack);
#endif // PBL_COLOR

  // Draw the hours.  We always use 12-hour time, because 12 Doctors.
  snprintf(buffer, buffer_size, "%d", (hour_value ? hour_value : 12));
  graphics_draw_text(ctx, buffer, font, hours_text_box,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentRight,
                     NULL);

  // Draw the (possibly blinking) colon.
  if (!config.second_hand || !hide_colon) {
    graphics_draw_text(ctx, ":", font, mins_hhmm_text_box,
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
                       NULL);
  }

  // draw minutes
  snprintf(buffer, buffer_size, " %02d", minute_value);
  graphics_draw_text(ctx, buffer, font, mins_hhmm_text_box,
                     GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft,
                     NULL);
}

void date_layer_update_callback(Layer *me, GContext* ctx) {
  //  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "date_layer: %d", config.show_date);
  if (config.show_date) {
    static const int buffer_size = 128;
    char buffer[buffer_size];
    GFont font;

    graphics_context_set_compositing_mode(ctx, GCompOpOr);

    if (date_background.bitmap == NULL) {
      date_background = rle_bwd_create(RESOURCE_ID_DATE_BACKGROUND);
    }
    if (date_background.bitmap != NULL) {
      graphics_draw_bitmap_in_rect(ctx, date_background.bitmap, date_background_box);
    }

    font = fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD);

    graphics_context_set_text_color(ctx, GColorBlack);
    const LangDef *lang = &lang_table[config.display_lang % num_langs];
    const char *weekday_name = lang->weekday_names[day_value];
    snprintf(buffer, buffer_size, "%s %d", weekday_name, date_value);
    graphics_draw_text(ctx, buffer, font, date_text_box,
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter,
                       NULL);
  }
}


void update_time(struct tm *tick_time, bool for_startup) {
  int face_new, hour_new, minute_new, second_new, day_new, date_new;

  hour_new = face_new = tick_time->tm_hour % 12;
  minute_new = tick_time->tm_min;
  second_new = tick_time->tm_sec;
  if (config.hurt && face_new == 8 && minute_new >= 30) {
    // Face 8.5 is John Hurt.
    face_new = 12;
  }
  day_new = tick_time->tm_wday;
  date_new = tick_time->tm_mday;
#ifdef FAST_TIME
  if (config.hurt) {
    int double_face = ((tick_time->tm_min * 60 + tick_time->tm_sec) / 3) % 24;
    hour_new = double_face / 2;
    if (double_face == 17) {
      face_new = 12;
    } else {
      face_new = double_face / 2;
    }
  } else {
    hour_new = face_new = ((tick_time->tm_min * 60 + tick_time->tm_sec) / 6) % 12;
  }
  minute_new = tick_time->tm_sec;
  day_new = ((tick_time->tm_min * 60 + tick_time->tm_sec) / 4) % 7;
  date_new = (tick_time->tm_min * 60 + tick_time->tm_sec) % 31 + 1;
#endif

  /*
  // Hack for screenshots.
  {
    face_new = hour_new = 10; minute_new = 9; // 10:09
    //face_new = hour_new = 11; minute_new = 2; // 11:02
    //face_new = hour_new = 4; minute_new = 15; // 4:15
    //face_new = hour_new = 1; minute_new = 30; // 1:30
    //face_new = hour_new = 2; minute_new = 39; // 2:39
  }
  */

  if (hour_new != hour_value) {
    // Update the hour display.
    hour_value = hour_new;
    layer_mark_dirty(hhmm_layer);
  }

  if (minute_new != minute_value) {
    // Update the minute display.
    minute_value = minute_new;
    layer_mark_dirty(mm_layer);
    layer_mark_dirty(hhmm_layer);
  }

  if (second_new != second_value) {
    // Update the second display.
    second_value = second_new;
    hide_colon = false;
    if (config.second_hand) {
      // To blink the colon once per second, draw it now, then make it
      // go away after a half-second.
      layer_mark_dirty(mm_layer);
      layer_mark_dirty(hhmm_layer);

      if (blink_timer != NULL) {
        app_timer_cancel(blink_timer);
        blink_timer = NULL;
      }
      blink_timer = app_timer_register(500, &handle_blink, 0);
    }
  }

  if (day_new != day_value || date_new != date_value) {
    // Update the day/date display.
    day_value = day_new;
    date_value = date_new;
    layer_mark_dirty(date_layer);
  }

  if (face_transition) {
    layer_mark_dirty(face_layer);
  } else if (face_new != face_value) {
    start_transition(face_new, for_startup);
  }

  set_next_timer();
}

// Update the watch as time passes.
void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (face_value == -1) {
    // We haven't loaded yet.
    return;
  }
  update_time(tick_time, false);
}

// Updates any runtime settings as needed when the config changes.
void apply_config() {
  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "apply_config, second_hand=%d", config.second_hand);
  tick_timer_service_unsubscribe();

#ifdef FAST_TIME
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
#else
  if (config.second_hand) {
    tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
  }
#endif

  if (config.show_hour) {
    layer_set_hidden(hhmm_layer, false);
    layer_set_hidden(mm_layer, true);
  } else {
    layer_set_hidden(hhmm_layer, true);
    layer_set_hidden(mm_layer, false);
  }

  refresh_battery_gauge();
  refresh_bluetooth_indicator();

  bwd_destroy(&mins_background);
  bwd_destroy(&hours_background);
  bwd_destroy(&date_background);
}

#if PBL_API_EXISTS(layer_get_unobstructed_bounds)
// The unobstructed area of the watchface is changing (e.g. due to a
// timeline quick view message).  Adjust layers accordingly.
void adjust_unobstructed_area() {
  struct Layer *root_layer = window_get_root_layer(window);
  GRect bounds = layer_get_unobstructed_bounds(root_layer);
  GRect orig_bounds = layer_get_bounds(root_layer);
  any_obstructed_area = (memcmp(&bounds, &orig_bounds, sizeof(bounds)) != 0);
  int bottom_shift = SCREEN_HEIGHT - bounds.size.h;

  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "unobstructed_area: %d %d %d %d, bottom_shift = %d, any_obstructed_area = %d", bounds.origin.x, bounds.origin.y, bounds.size.w, bounds.size.h, bottom_shift, any_obstructed_area);

  // Shift everything on the bottom of the screen up by the
  // appropriate amount.
  GRect mm_layer_shifted = mm_layer_box;
  GRect hhmm_layer_shifted = hhmm_layer_box;
  GRect date_layer_shifted = date_layer_box;

  mm_layer_shifted.origin.y -= bottom_shift;
  hhmm_layer_shifted.origin.y -= bottom_shift;
  date_layer_shifted.origin.y -= bottom_shift;

  layer_set_frame(mm_layer, mm_layer_shifted);
  layer_set_frame(hhmm_layer, hhmm_layer_shifted);
  layer_set_frame(date_layer, date_layer_shifted);

  // Shift the face layer to center the face within the new region.
  int cx = bounds.origin.x + bounds.size.w / 2;
  int cy = bounds.origin.y + bounds.size.h / 2;

  GRect face_layer_shifted = { { cx - SCREEN_WIDTH / 2, cy - SCREEN_HEIGHT / 2 },
                               { SCREEN_WIDTH, SCREEN_HEIGHT } };
  layer_set_frame(face_layer, face_layer_shifted);
}

void unobstructed_area_change_handler(AnimationProgress progress, void *context) {
  adjust_unobstructed_area();
}
#endif  // PBL_API_EXISTS(layer_get_unobstructed_bounds)


void handle_init() {
  load_config();

  app_message_register_inbox_received(receive_config_handler);
  app_message_register_inbox_dropped(dropped_config_handler);

#define INBOX_MESSAGE_SIZE 200
#define OUTBOX_MESSAGE_SIZE 50

#ifndef NDEBUG
  uint32_t inbox_max = app_message_inbox_size_maximum();
  uint32_t outbox_max = app_message_outbox_size_maximum();
  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "available message space %u, %u", (unsigned int)inbox_max, (unsigned int)outbox_max);
  if (inbox_max > INBOX_MESSAGE_SIZE) {
    inbox_max = INBOX_MESSAGE_SIZE;
  }
  if (outbox_max > OUTBOX_MESSAGE_SIZE) {
    outbox_max = OUTBOX_MESSAGE_SIZE;
  }
  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "app_message_open(%u, %u)", (unsigned int)inbox_max, (unsigned int)outbox_max);
  AppMessageResult open_result = app_message_open(inbox_max, outbox_max);
  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "open_result = %d", open_result);

#else  // NDEBUG
  app_message_open(INBOX_MESSAGE_SIZE, OUTBOX_MESSAGE_SIZE);
#endif  // NDEBUG

  face_transition = false;
  face_value = -1;
  next_face_value = -1;
  next_face_slice = -1;
  last_buzz_hour = -1;
  hour_value = -1;
  minute_value = -1;
  second_value = -1;
  day_value = -1;
  date_value = -1;
  hide_colon = false;

  for (int si = 0; si < NUM_SLICES; ++si) {
    visible_face[si].face_value = -1;
  }

  window = window_create();
  window_set_background_color(window, GColorWhite);
  struct Layer *root_layer = window_get_root_layer(window);
  layer_set_update_proc(root_layer, &root_layer_update_callback);

  window_stack_push(window, true);

  face_layer = layer_create(layer_get_bounds(root_layer));
  layer_set_update_proc(face_layer, &face_layer_update_callback);
  layer_add_child(root_layer, face_layer);

  mm_layer = layer_create(mm_layer_box);
  layer_set_update_proc(mm_layer, &mm_layer_update_callback);
  layer_add_child(root_layer, mm_layer);

  hhmm_layer = layer_create(hhmm_layer_box);
  layer_set_update_proc(hhmm_layer, &hhmm_layer_update_callback);
  layer_add_child(root_layer, hhmm_layer);

  date_layer = layer_create(date_layer_box);
  layer_set_update_proc(date_layer, &date_layer_update_callback);
  layer_add_child(root_layer, date_layer);

#ifdef PBL_ROUND
  init_bluetooth_indicator(root_layer, 10, 42);
  init_battery_gauge(root_layer, 144, 46);
#else  // PBL_ROUND
  init_bluetooth_indicator(root_layer, 0, 0);
  init_battery_gauge(root_layer, 119, 0);
#endif  // PBL_ROUND

#if PBL_API_EXISTS(layer_get_unobstructed_bounds)
  struct UnobstructedAreaHandlers unobstructed_area_handlers;
  memset(&unobstructed_area_handlers, 0, sizeof(unobstructed_area_handlers));
  unobstructed_area_handlers.change = unobstructed_area_change_handler;
  unobstructed_area_service_subscribe(unobstructed_area_handlers, NULL);
  adjust_unobstructed_area();
#endif  // PBL_API_EXISTS(layer_get_unobstructed_bounds)

  time_t now = time(NULL);
  struct tm *startup_time = localtime(&now);
  srand(now);

  update_time(startup_time, true);

  apply_config();
}

void handle_deinit() {
  tick_timer_service_unsubscribe();
  stop_transition();

  window_stack_pop_all(false);  // Not sure if this is needed?
  layer_destroy(mm_layer);
  layer_destroy(hhmm_layer);
  layer_destroy(face_layer);
  window_destroy(window);

  for (int si = 0; si < NUM_SLICES; ++si) {
    bwd_destroy(&visible_face[si].face_image);
  }
  bwd_destroy(&date_background);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
