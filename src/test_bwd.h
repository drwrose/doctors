#ifndef TEST_BWD_H
#define TEST_BWD_H

// This file is a mock stand-in to simulate some of the Pebble SDK
// interfaces--particularly using resources--in a desktop environment,
// for the sole purpose of making it easier to debug the code in
// bwd.c.

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#define SUPPORT_RLE 1

typedef enum {
  APP_LOG_LEVEL_ERROR = 1,
  APP_LOG_LEVEL_WARNING = 50,
  APP_LOG_LEVEL_INFO = 100,
  APP_LOG_LEVEL_DEBUG = 200,
  APP_LOG_LEVEL_DEBUG_VERBOSE = 255,
} AppLogLevel;

void app_log(uint8_t log_level, const char* src_filename, int src_line_number, const char* fmt, ...)
    __attribute__((format(printf, 4, 5)));

typedef struct GSize {
  int16_t w;
  int16_t h;
} GSize;

typedef struct GPoint {
  int16_t x;
  int16_t y;
} GPoint;

typedef struct GRect {
  GPoint origin;
  GSize size;
} GRect;
#define GRect(x, y, w, h) ((GRect){{(x), (y)}, {(w), (h)}})

typedef struct GBitmap {
} GBitmap;

static GBitmap *gbitmap_create_with_resource(int resource_id) {
  return NULL;
}
static GBitmap *gbitmap_create_with_data(void *bitmap_data) {
  return NULL;
}
static void gbitmap_destroy(GBitmap *bitmap) { }
static GRect gbitmap_get_bounds(GBitmap *bitmap) { return GRect(0, 0, 0, 0); }
static int gbitmap_get_bytes_per_row(GBitmap *bitmap) { return 0; }
static void *gbitmap_get_data(GBitmap *bitmap) { return NULL; }

static size_t heap_bytes_used(void) { return 0; }
static size_t heap_bytes_free(void) { return 0; }

// Here's where we mock up the resource stuff.
typedef struct ResHandle {
  size_t _size;
  uint8_t *_data;
} ResHandle;

ResHandle resource_get_handle(int resource_id);
size_t resource_load_byte_range(ResHandle h, uint32_t start_offset, uint8_t *buffer, size_t num_bytes);
static size_t resource_size(ResHandle h) { return h._size; }

#endif 
