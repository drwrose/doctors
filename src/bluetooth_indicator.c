#include <pebble.h>
#include "bluetooth_indicator.h"
#include "config_options.h"
#include "bwd.h"

BitmapWithData bluetooth_disconnected;
BitmapWithData bluetooth_connected;
BitmapWithData bluetooth_mask;
Layer *bluetooth_layer;
bool bluetooth_state = false;

//#define DYNAMIC_INVERT

#ifdef DYNAMIC_INVERT

// On monochrome watches, these parameters are passed in.
bool bluetooth_invert = false;

#else  // DYNAMIC_INVERT

// On color watches, the icon is never inverted.
#define bluetooth_invert false

#endif  // DYNAMIC_INVERT


void bluetooth_layer_update_callback(Layer *me, GContext *ctx) {
  if (config.bluetooth_indicator == IM_off) {
    return;
  }

  GRect box = layer_get_frame(me);
  box.origin.x = 0;
  box.origin.y = 0;

  GCompOp fg_mode;

#ifdef PBL_BW
  GCompOp mask_mode;
  if (bluetooth_invert) {
    fg_mode = GCompOpSet;
    mask_mode = GCompOpAnd;
  } else {
    fg_mode = GCompOpAnd;
    mask_mode = GCompOpSet;
  }
#else  // PBL_BW
  // On color watches, we always use GCompOpSet because the icon
  // includes its own alpha channel.
  fg_mode = GCompOpSet;
#endif  // PBL_BW

  bool new_state = bluetooth_connection_service_peek();
  if (new_state != bluetooth_state) {
    bluetooth_state = new_state;
    if (config.bluetooth_buzzer && !bluetooth_state) {
      // We just lost the bluetooth connection.  Ring the buzzer.
      vibes_short_pulse();
    }
  }

  if (bluetooth_state) {
    if (config.bluetooth_indicator != IM_when_needed) {
      // We don't draw the "connected" bitmap if bluetooth_indicator
      // is set to IM_when_needed; only on IM_always.
#ifdef PBL_BW
      if (bluetooth_mask.bitmap == NULL) {
        bluetooth_mask = png_bwd_create(RESOURCE_ID_BLUETOOTH_MASK);
      }
      graphics_context_set_compositing_mode(ctx, mask_mode);
      graphics_draw_bitmap_in_rect(ctx, bluetooth_mask.bitmap, box);
#endif  // PBL_BW
      if (bluetooth_connected.bitmap == NULL) {
        bluetooth_connected = png_bwd_create(RESOURCE_ID_BLUETOOTH_CONNECTED);
      }
      graphics_context_set_compositing_mode(ctx, fg_mode);
      graphics_draw_bitmap_in_rect(ctx, bluetooth_connected.bitmap, box);
    }
  } else {
    // We always draw the disconnected bitmap (except in the IM_off
    // case, of course).
#ifdef PBL_BW
    if (bluetooth_mask.bitmap == NULL) {
      bluetooth_mask = png_bwd_create(RESOURCE_ID_BLUETOOTH_MASK);
    }
    graphics_context_set_compositing_mode(ctx, mask_mode);
    graphics_draw_bitmap_in_rect(ctx, bluetooth_mask.bitmap, box);
#endif  // PBL_BW
    if (bluetooth_disconnected.bitmap == NULL) {
      bluetooth_disconnected = png_bwd_create(RESOURCE_ID_BLUETOOTH_DISCONNECTED);
    }
    graphics_context_set_compositing_mode(ctx, fg_mode);
    graphics_draw_bitmap_in_rect(ctx, bluetooth_disconnected.bitmap, box);
  }
}

// Update the bluetooth guage.
void handle_bluetooth(bool connected) {
  layer_mark_dirty(bluetooth_layer);
}

void init_bluetooth_indicator(Layer *window_layer) {
#ifdef DYNAMIC_INVERT
  bluetooth_invert = false;
#endif  // DYNAMIC_INVERT
  bluetooth_layer = layer_create(GRect(0, 0, 18, 18));
  layer_set_update_proc(bluetooth_layer, &bluetooth_layer_update_callback);
  layer_add_child(window_layer, bluetooth_layer);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
}

void move_bluetooth_indicator(int x, int y, bool invert) {
#ifdef DYNAMIC_INVERT
  bluetooth_invert = invert;
#endif  // DYNAMIC_INVERT
  layer_set_frame((Layer *)bluetooth_layer, GRect(x, y, 18, 18));
}

void deinit_bluetooth_indicator() {
  bluetooth_connection_service_unsubscribe();
  layer_destroy(bluetooth_layer);
  bluetooth_layer = NULL;
  bwd_destroy(&bluetooth_disconnected);
  bwd_destroy(&bluetooth_connected);
  bwd_destroy(&bluetooth_mask);
}

void refresh_bluetooth_indicator() {
  layer_mark_dirty(bluetooth_layer);
}
