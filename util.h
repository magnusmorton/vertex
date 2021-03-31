#ifndef MAG_UTIL_H
#define MAG_UTIL_H

#include <stdlib.h>

struct array {
  size_t capacity, length, element_size;
  void *data;
};

typedef struct array mag_array;

int init_array(mag_array*, size_t, size_t);

void* array_get(mag_array*, size_t);

void array_put(mag_array*, size_t, void*);

int array_push(mag_array*, void*);

void free_array(mag_array*);


#endif
