#include "assert.h"

#ifndef NDEBUG
static jmp_buf *assert_env;

void setup_assert(jmp_buf *env) {
  assert_env = env;
}

void assert_failure(const char *condition, const char *filename, int line_number) {
  app_log(APP_LOG_LEVEL_ERROR, filename, line_number, "assertion failed: %s", condition);

  // Force a crash.
  //char *null_ptr = 0;
  //(*null_ptr) = 0;

  // Try returning back to main.
  //  longjmp(*assert_env, 1);
}
#endif  // NDEBUG
