#include <stdio.h>
#include <stdlib.h>

#include "runtime.h"

int main() {
  int *arr = malloc(sizeof(int) * 128);
  int foo = 34;
  int bar = arr[23];
  int arr2[97];
  arr2[45] = 39;
}
