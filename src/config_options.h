#ifndef CONFIG_OPTIONS_H
#define CONFIG_OPTIONS_H

#include <pebble.h>

// These keys are used to communicate with Javascript.
typedef enum {
  CK_keep_battery_gauge = 0,
  CK_keep_bluetooth_indicator = 1,
  CK_second_hand = 2,
  CK_hour_buzzer = 3,
  CK_hurt = 4,
} ConfigKey;

// This key is used to record the persistent storage.
#define PERSIST_KEY 0x5150

typedef struct {
  bool keep_battery_gauge;
  bool keep_bluetooth_indicator;
  bool second_hand;
  bool hour_buzzer;
  bool hurt;
} __attribute__((__packed__)) ConfigOptions;

extern ConfigOptions config;

void init_default_options();
void save_config();
void load_config();
void receive_config_handler(DictionaryIterator *received, void *context);

void apply_config();  // implemented in the main program

#endif  // CONFIG_OPTIONS_H
