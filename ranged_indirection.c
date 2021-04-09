#include <stdio.h>

int main()
{
  int a[128];
  int b[128];
  int tmp = 0;
  // init b
  for (int i=0; i < 128; i++) {
    b[i] = i;
  }
  for (int i=0; i < 127; i++) {
    for (int j=b[i]; j < b[i+1]; j++) {
      tmp += a[j%128];
    }
  }
  printf("tmp is %d\n", tmp);
}
