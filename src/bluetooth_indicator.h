#ifndef BLUETOOTH_INDICATOR_H
#define BLUETOOTH_INDICATOR_H

void init_bluetooth_indicator(Layer *window_layer, int x, int y);
void deinit_bluetooth_indicator();
void refresh_bluetooth_indicator();
void poll_quiet_time_state();

#endif  // BLUETOOTH_INDICATOR_H
