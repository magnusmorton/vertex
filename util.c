#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"

struct list_elem {
  struct list_elem *forward;
  struct list_elem *back;
  int elem;
};

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

struct array make_adj_list() {
  struct array adj_list;
  init_array(&adj_list, 512, sizeof(struct list_elem *));
  return adj_list;
}

void destroy_adj_list(struct array *list) {
  for (int i = 0; i < list->length; i++) {
    struct list_elem *el = array_get(list, i);
    while (el) {
      struct list_elem *next = el->forward;
      free(el);
      el = next;
    }
  }
  free(list);
}

int add_edge(struct array *adj_list, int source, int sink) {
  int rc = 0;
  // find source;
  struct list_elem *list = array_get(adj_list, source);

  // if existing edge
  if (list) {
    for (struct list_elem *el = list; list != NULL; list = list->forward) {
      printf("el: %p\n", el);
    }
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
