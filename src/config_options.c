#include "config_options.h"

ConfigOptions config;

void init_default_options() {
  // Initializes the config options with their default values.  Note
  // that these defaults are used only if the Pebble is not connected
  // to the phone at the time of launch; otherwise, the defaults in
  // pebble-js-app.js are used instead.
  config.keep_battery_gauge = false;
  config.keep_bluetooth_indicator = false;
  config.second_hand = false;
  config.hour_buzzer = false;
  config.hurt = true;
}

const char *show_config() {
  static char buffer[48];
  snprintf(buffer, 48, "bat: %d, bt: %d, sh: %d, hb: %d, h: %d", config.keep_battery_gauge, config.keep_bluetooth_indicator, config.second_hand, config.hour_buzzer, config.hurt);
  return buffer;
}

void save_config() {
  int wrote = persist_write_data(PERSIST_KEY, &config, sizeof(config));
  if (wrote == sizeof(config)) {
    app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "Saved config (%d, %d): %s", PERSIST_KEY, sizeof(config), show_config());
  } else {
    app_log(APP_LOG_LEVEL_ERROR, __FILE__, __LINE__, "Error saving config (%d, %d): %d", PERSIST_KEY, sizeof(config), wrote);
  }
}

void load_config() {
  init_default_options();

  ConfigOptions local_config;
  int read_size = persist_read_data(PERSIST_KEY, &local_config, sizeof(local_config));
  if (read_size == sizeof(local_config)) {
    config = local_config;
    app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "Loaded config (%d, %d): %s", PERSIST_KEY, sizeof(config), show_config());
  } else {
    app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "No previous config (%d, %d): %d", PERSIST_KEY, sizeof(config), read_size);
  }
}

void receive_config_handler(DictionaryIterator *received, void *context) {
  app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "receive_config_handler");
  ConfigOptions orig_config = config;

  Tuple *keep_battery_gauge = dict_find(received, CK_keep_battery_gauge);
  if (keep_battery_gauge != NULL) {
    config.keep_battery_gauge = keep_battery_gauge->value->int32;
  }

  Tuple *keep_bluetooth_indicator = dict_find(received, CK_keep_bluetooth_indicator);
  if (keep_bluetooth_indicator != NULL) {
    config.keep_bluetooth_indicator = keep_bluetooth_indicator->value->int32;
  }

  Tuple *second_hand = dict_find(received, CK_second_hand);
  if (second_hand != NULL) {
    config.second_hand = second_hand->value->int32;
  }

  Tuple *hour_buzzer = dict_find(received, CK_hour_buzzer);
  if (hour_buzzer != NULL) {
    config.hour_buzzer = hour_buzzer->value->int32;
  }

  Tuple *hurt = dict_find(received, CK_hurt);
  if (hurt != NULL) {
    config.hurt = hurt->value->int32;
  }

  app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "New config: %s", show_config());
  if (memcmp(&orig_config, &config, sizeof(config)) == 0) {
    app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "Config is unchanged.");
  } else {
    save_config();
    apply_config();
  }
}
