#include <sanitizer/dfsan_interface.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  int a[128], b[128];
  memset(a, 7, 128 * sizeof(int));
  dfsan_label a_label = dfsan_create_label("a", 0);
  dfsan_add_label(a_label, a, sizeof(a));
  dfsan_label b_label = dfsan_create_label("b", 0);
  dfsan_add_label(b_label, b, sizeof(b));
  int tmp = 0;
  // init b
  dfsan_label check = dfsan_read_label(b+2, sizeof(int));
  assert(dfsan_has_label(check,b_label));
  for (int i=0; i < 128; i++) {
    b[i] = i;
  }
  for (int i=0; i < 128; i++) {
    tmp += a[b[i]];
    check = dfsan_read_label(b, sizeof(b));
//    assert(dfsan_has_label(check,a_label));
    assert(dfsan_has_label(check,b_label));
  }
  printf("tmp is %d\n", tmp);

  return 0;
}
