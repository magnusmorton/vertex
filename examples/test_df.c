#include <stdio.h>
#include <stdlib.h>

#include "runtime.h"

int main() {
  init_san();
  int *arr = malloc(sizeof(int) * 128);
  _create_label("arr", arr, sizeof(int) * 128);
  int foo = 34;
  _check_ptr(&foo);

  int bar = arr[23];
  _check_ptr(&bar);
  int arr2[97];
  _create_label("arr2", arr2, sizeof(int) * 128);
  arr2[45] = 39;
  _check_ptr(arr2+45);
  finish_san();
}
