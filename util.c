#include <stdlib.h>
#include <string.h>

#include "util.h"

int init_array(struct array *arr, size_t capacity, size_t element_size) {
  int rc = 1;
  void *temp = malloc(capacity * element_size);
  if (temp) {
    arr->data = temp;
    arr->capacity = capacity;
    arr->length = 0;
    arr->element_size = element_size;
    rc = 0;
  }

  return rc;
}


void* array_get(struct array *arr, size_t index) {
  return arr->data + index * arr->element_size;
}

void array_put(struct array *arr, size_t index, void *data) {
  memcpy(arr->data + (index * arr->element_size), data, arr->element_size);
}

int array_push(struct array *arr, void *data) {
  size_t index = arr->length;
  if (index == arr->capacity) {
    void *temp = realloc(arr->data, (arr->capacity * 2) * arr->element_size);
    if (temp) {
      arr->data = temp;
      arr->capacity = arr->capacity * 2;
    } else {
      return 1;
    } 
  }
  
  array_put(arr, index, data);
  arr->length++;
  
  return 0;
}

void free_array(struct array *arr) {
  free(arr->data);
}
