#ifndef DOCTORS_H
#define DOCTORS_H

#include <pebble.h>

#include "../resources/generated_config.h"

#ifdef PBL_PLATFORM_APLITE
#define gbitmap_get_bounds(bm) ((bm)->bounds)
#define gbitmap_get_bytes_per_row(bm) ((bm)->row_size_bytes)
#define gbitmap_get_data(bm) ((bm)->addr)
#define gbitmap_get_format(bm) (0)
#endif  // PBL_PLATFORM_APLITE

#endif

