#include <stdio.h>
#include <stdlib.h>

int main() {
  int *arr = malloc(sizeof(int) * 128);
  printf("mallocd at %p\n", arr);
}
