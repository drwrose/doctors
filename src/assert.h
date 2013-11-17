#ifndef __assert_h
#define __assert_h

// Our own poor-man's assert() function, since Pebble doesn't provide one.
#define assert(condition) { \
  if (!(condition)) { \
    uint8_t *bad_ptr = 0; \
    (*bad_ptr) = 0xff; \
  } \
  }

#endif  // __assert_h
