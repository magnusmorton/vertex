#include <glib.h>

#include "runtime.h"

extern void _mark_root(const char* label, void *ptr, size_t size, const char* file, unsigned line);

static void test_no_data() {
  // do nothing
  
  g_assert_true(detect(NULL) == 0);
}

static void test_1_data() {
  void *ptr = 0x94;
  _mark_root("foo", ptr, 8, NULL, 0);
  g_assert_true(detect(NULL) == 1);
}

int main(int argc, char *argv[]) {
  g_test_init (&argc, &argv, NULL);
  g_test_add_func("/basic/no_data", test_no_data);
  g_test_add_func("/basic/one_data", test_1_data);
  return g_test_run();
}
