#include <stdio.h>
#include <stdlib.h>

int main()
{
  int *a = (int*)malloc(sizeof(int) * 128);
  int *b = (int*)malloc(sizeof(int) * 128);
  int tmp = 0;
  // init b
  for (int i=0; i < 128; i++) {
    a[i] = 4;
    b[i] = i;
  }
  for (int i=0; i < 128; i++) {
    int idx = b[i];
    tmp += a[idx];
  }
  printf("tmp is %d\n", tmp);
}
