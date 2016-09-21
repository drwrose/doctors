#include "doctors.h"
#include <pebble.h>
#include "bluetooth_indicator.h"
#include "config_options.h"
#include "bwd.h"

BitmapWithData bluetooth_disconnected;
BitmapWithData bluetooth_connected;
BitmapWithData bluetooth_mask;
Layer *bluetooth_layer;
bool bluetooth_state = false;

void bluetooth_layer_update_callback(Layer *me, GContext *ctx) {
  if (config.bluetooth_indicator == IM_off) {
    return;
  }

  GRect box = layer_get_frame(me);
  box.origin.x = 0;
  box.origin.y = 0;

  GCompOp fg_mode;

#ifdef PBL_BW
  fg_mode = GCompOpAnd;
  GCompOp mask_mode = GCompOpSet;
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
        bluetooth_mask = rle_bwd_create(RESOURCE_ID_BLUETOOTH_MASK);
      }
      graphics_context_set_compositing_mode(ctx, mask_mode);
      graphics_draw_bitmap_in_rect(ctx, bluetooth_mask.bitmap, box);
#endif  // PBL_BW
      if (bluetooth_connected.bitmap == NULL) {
        bluetooth_connected = rle_bwd_create(RESOURCE_ID_BLUETOOTH_CONNECTED);
      }
      graphics_context_set_compositing_mode(ctx, fg_mode);
      graphics_draw_bitmap_in_rect(ctx, bluetooth_connected.bitmap, box);
    }
  } else {
    // We always draw the disconnected bitmap (except in the IM_off
    // case, of course).
#ifdef PBL_BW
    if (bluetooth_mask.bitmap == NULL) {
      bluetooth_mask = rle_bwd_create(RESOURCE_ID_BLUETOOTH_MASK);
    }
    graphics_context_set_compositing_mode(ctx, mask_mode);
    graphics_draw_bitmap_in_rect(ctx, bluetooth_mask.bitmap, box);
#endif  // PBL_BW
    if (bluetooth_disconnected.bitmap == NULL) {
      bluetooth_disconnected = rle_bwd_create(RESOURCE_ID_BLUETOOTH_DISCONNECTED);
    }
    graphics_context_set_compositing_mode(ctx, fg_mode);
    graphics_draw_bitmap_in_rect(ctx, bluetooth_disconnected.bitmap, box);
  }
}

// Update the bluetooth guage.
void handle_bluetooth(bool connected) {
  layer_mark_dirty(bluetooth_layer);
}

void init_bluetooth_indicator(Layer *window_layer, int x, int y) {
  bluetooth_layer = layer_create(GRect(x, y, BLUETOOTH_SIZE_X, BLUETOOTH_SIZE_Y));
  layer_set_update_proc(bluetooth_layer, &bluetooth_layer_update_callback);
  layer_add_child(window_layer, bluetooth_layer);
  bluetooth_connection_service_subscribe(&handle_bluetooth);
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
