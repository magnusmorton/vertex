#include <sanitizer/dfsan_interface.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
  int a[128] = {0};
  int b = 6;

  dfsan_label a_label = dfsan_create_label("a", 0);
  dfsan_add_label(a_label, &a, sizeof(a));
  dfsan_label b_label = dfsan_create_label("b", 0);
  dfsan_add_label(b_label, &b, sizeof(b));

  a[10] = b; 
  dfsan_label check = dfsan_read_label(&a, sizeof(a));

  assert(dfsan_has_label(check,b_label));
  assert(dfsan_has_label(check,a_label));


  return 0;
}
