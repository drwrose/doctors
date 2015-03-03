#ifndef TEST_BWD_H
#define TEST_BWD_H

// This file is a mock stand-in to simulate some of the Pebble SDK
// interfaces--particularly using resources--in a desktop environment,
// for the sole purpose of making it easier to debug the code in
// bwd.c.

#include <assert.h>
#include <stdio.h>
#include <malloc.h>

#define SUPPORT_RLE 1

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

typedef struct GBitmap {
} GBitmap;

static GBitmap *gbitmap_create_with_resource(int resource_id) {
  return NULL;
}
static void gbitmap_destroy(GBitmap *bitmap) { }
static GRect gbitmap_get_bounds(GBitmap *bitmap) { return GRect(); }
static int gbitmap_get_bytes_per_row(GBitmap *bitmap) { return 0; }
static void *gbitmap_get_data(GBitmap *bitmap) { return NULL; }

static size_t heap_bytes_used() { return 0; }
static size_t heap_bytes_free() { return 0; }

// Here's where we mock up the resource stuff.
typedef struct ResHandle {
  size_t _size;
  uint8_t *_data;
} ResHandle;

ResHandle resource_get_handle(int resource_id);
size_t resource_load_byte_range(ResHandle h, uint32_t start_offset, uint8_t *buffer, size_t num_bytes);
size_t resource_size(ResHandle h) { return h._size; }

#endif 
