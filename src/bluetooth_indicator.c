#include "doctors.h"
#include <pebble.h>
#include "bluetooth_indicator.h"
#include "config_options.h"
#include "bwd.h"
#include "qapp_log.h"

BitmapWithData bluetooth_disconnected;
BitmapWithData bluetooth_connected;
BitmapWithData bluetooth_mask;
Layer *bluetooth_layer;
bool got_bluetooth_state = false;
bool bluetooth_state;
bool bluetooth_buzzed_state = false;

#ifdef PBL_PLATFORM_APLITE
// On Aplite, quiet_time_is_active() always returns false.  Might as
// well avoid the extra code to support it then.
#define quiet_time_state false

#else // PBL_PLATFORM_APLITE
BitmapWithData quiet_time;
BitmapWithData quiet_time_mask;
bool quiet_time_state = false;

#endif  // PBL_PLATFORM_APLITE

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

  poll_quiet_time_state();  // Just in case it's recently changed.
  if (!got_bluetooth_state) {
    bluetooth_state = bluetooth_connection_service_peek();
    got_bluetooth_state = true;
  }
  if (bluetooth_state != bluetooth_buzzed_state) {
    bluetooth_buzzed_state = bluetooth_state;
    if (config.bluetooth_buzzer && !bluetooth_buzzed_state) {
      // We just lost the bluetooth connection.  Ring the buzzer, if
      // it isn't quiet time.
      if (!quiet_time_state) {
        vibes_short_pulse();
      }
    }
  }

  if (bluetooth_buzzed_state) {
    // Bluetooth is connected.
    if (quiet_time_state) {
      // If bluetooth is connected and quiet time is enabled, we draw
      // the "quiet time" bitmap.
#ifndef PBL_PLATFORM_APLITE
#ifdef PBL_BW
      if (quiet_time_mask.bitmap == NULL) {
        quiet_time_mask = rle_bwd_create(RESOURCE_ID_QUIET_TIME_MASK);
      }
      graphics_context_set_compositing_mode(ctx, mask_mode);
      graphics_draw_bitmap_in_rect(ctx, quiet_time_mask.bitmap, box);
#endif  // PBL_BW
      if (quiet_time.bitmap == NULL) {
        quiet_time = rle_bwd_create(RESOURCE_ID_QUIET_TIME);
      }
      graphics_context_set_compositing_mode(ctx, fg_mode);
      graphics_draw_bitmap_in_rect(ctx, quiet_time.bitmap, box);
#endif // PBL_PLATFORM_APLITE

    } else {
      // If bluetooth is connected and quiet time is not enabled, we
      // draw the "connected" bitmap, unless bluetooth_indicator is
      // set to IM_when_needed.
      if (config.bluetooth_indicator != IM_when_needed) {
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
    }
  } else {
    // If bluetooth is disconnected, we draw the "disconnected" bitmap
    // (except in the IM_off case, of course).
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
static void handle_bluetooth(bool new_bluetooth_state) {
  if (got_bluetooth_state && bluetooth_state == new_bluetooth_state) {
    // No change; ignore the update.
    qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "bluetooth update received, no change to bluetooth");
    return;
  }

  bluetooth_state = new_bluetooth_state;
  got_bluetooth_state = true;
  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "bluetooth changed to %d", bluetooth_state);

  if (config.bluetooth_indicator != IM_off) {
    layer_mark_dirty(bluetooth_layer);
  }
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

#ifndef PBL_PLATFORM_APLITE
  bwd_destroy(&quiet_time);
  bwd_destroy(&quiet_time_mask);
#endif  // PBL_PLATFORM_APLITE
}

void refresh_bluetooth_indicator() {
  layer_mark_dirty(bluetooth_layer);
}

// We have to poll the quiet_time_is_active() state from time to
// time because Pebble doesn't provide a callback handler for this.
void poll_quiet_time_state() {
#ifndef PBL_PLATFORM_APLITE
  bool new_quiet_time_state = quiet_time_is_active();
  if (quiet_time_state == new_quiet_time_state) {
    // No change.
    return;
  }

  quiet_time_state = new_quiet_time_state;
  qapp_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "quiet_time changed to %d", quiet_time_state);

  if (config.bluetooth_indicator != IM_off) {
    layer_mark_dirty(bluetooth_layer);
  }
#endif  // PBL_PLATFORM_APLITE
}
