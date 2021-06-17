#include <stdio.h>
#include <stdlib.h>

int main() {
  int *arr = malloc(sizeof(int) * 128);
  int arr2[97];
  arr2[45] = arr + 3;
  free(arr);
}
