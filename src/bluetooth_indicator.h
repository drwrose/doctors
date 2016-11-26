#ifndef BLUETOOTH_INDICATOR_H
#define BLUETOOTH_INDICATOR_H

void init_bluetooth_indicator(Layer *window_layer, int x, int y);
void deinit_bluetooth_indicator();
void refresh_bluetooth_indicator();

#ifdef PBL_PLATFORM_APLITE
#define poll_quiet_time_state() (false)
#else  // PBL_PLATFORM_APLITE
bool poll_quiet_time_state();
#endif  // PBL_PLATFORM_APLITE

#endif  // BLUETOOTH_INDICATOR_H
