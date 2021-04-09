#include <stdio.h>

int main()
{
  int a[128], b[128];
  int tmp = 0;
  // init b
  for (int i=0; i < 128; i++) {
    b[i] = i;
  }
  for (int i=0; i < 128; i++) {
    tmp += a[b[i]];
  }
  printf("tmp is %d\n", tmp);
}
