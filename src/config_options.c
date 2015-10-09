#include "config_options.h"
#include "lang_table.h"

ConfigOptions config;

void init_default_options() {
  // Initializes the config options with their default values.  Note
  // that these defaults are used only if the Pebble is not connected
  // to the phone at the time of launch; otherwise, the defaults in
  // pebble-js-app.js are used instead.
  static ConfigOptions default_options = {
    IM_when_needed,   // battery_gauge
    IM_when_needed,   // bluetooth_indicator
    false,            // second_hand
    false,            // hour_buzzer
    true,             // bluetooth_buzzer
    true,             // hurt
    false,            // show_date
    false,            // show_hour
    DL_english,       // display_lang
  };
  
  config = default_options;
}

void sanitize_config() {
  // Ensures that the newly-loaded config parameters are within a
  // reasonable range for the program and won't cause crashes.
  config.battery_gauge = config.battery_gauge % (IM_digital + 1);
  config.bluetooth_indicator = config.bluetooth_indicator % (IM_always + 1);
  config.display_lang = config.display_lang % (DL_num_languages);
}

void save_config() {
  int wrote = persist_write_data(PERSIST_KEY, &config, sizeof(config));
  if (wrote == sizeof(config)) {
    app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "Saved config (%d, %d)", PERSIST_KEY, sizeof(config));
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
    app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "Loaded config (%d, %d)", PERSIST_KEY, sizeof(config));
  } else {
    app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "No previous config (%d, %d): %d", PERSIST_KEY, sizeof(config), read_size);
  }

  sanitize_config();
}

void dropped_config_handler(AppMessageResult reason, void *context) {
  app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "dropped message: 0x%04x", reason);
}

void receive_config_handler(DictionaryIterator *received, void *context) {
  app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "receive_config_handler");
  ConfigOptions orig_config = config;

  Tuple *battery_gauge = dict_find(received, CK_battery_gauge);
  if (battery_gauge != NULL) {
    config.battery_gauge = (IndicatorMode)battery_gauge->value->int32;
  }

  Tuple *bluetooth_indicator = dict_find(received, CK_bluetooth_indicator);
  if (bluetooth_indicator != NULL) {
    config.bluetooth_indicator = (IndicatorMode)bluetooth_indicator->value->int32;
  }

  Tuple *second_hand = dict_find(received, CK_second_hand);
  if (second_hand != NULL) {
    config.second_hand = second_hand->value->int32;
  }

  Tuple *hour_buzzer = dict_find(received, CK_hour_buzzer);
  if (hour_buzzer != NULL) {
    config.hour_buzzer = hour_buzzer->value->int32;
  }

  Tuple *bluetooth_buzzer = dict_find(received, CK_bluetooth_buzzer);
  if (bluetooth_buzzer != NULL) {
    config.bluetooth_buzzer = bluetooth_buzzer->value->int32;
  }

  Tuple *hurt = dict_find(received, CK_hurt);
  if (hurt != NULL) {
    config.hurt = hurt->value->int32;
  }

  Tuple *show_date = dict_find(received, CK_show_date);
  if (show_date != NULL) {
    config.show_date = show_date->value->int32;
  }

  Tuple *show_hour = dict_find(received, CK_show_hour);
  if (show_hour != NULL) {
    config.show_hour = show_hour->value->int32;
  }

  Tuple *display_lang = dict_find(received, CK_display_lang);
  if (display_lang != NULL) {
    // Look for the matching language name in our table of known languages.
    for (int li = 0; li < num_langs; ++li) {
      if (strcmp(display_lang->value->cstring, lang_table[li].locale_name) == 0) {
	config.display_lang = li;
	break;
      }
    }
  }

  sanitize_config();

  app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "New config, display_lang = %d", config.display_lang);
  if (memcmp(&orig_config, &config, sizeof(config)) == 0) {
    app_log(APP_LOG_LEVEL_INFO, __FILE__, __LINE__, "Config is unchanged.");
  } else {
    save_config();
    apply_config();
  }
}
