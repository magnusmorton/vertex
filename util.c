#include <assert.h>
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
  int rc = init_array(&adj_list, 512, sizeof(struct list_elem *));
  assert(rc == 0);
  memset(adj_list.data, 0, 512 * sizeof(struct list_elem *));
  return adj_list;
}

void destroy_adj_list(struct array *list) {
  for (unsigned i = 0; i < list->length; i++) {
    struct list_elem **elp = array_get(list, i);
    struct list_elem *el = *elp;
    while (el) {
      struct list_elem *next = el->forward;
      free(el);
      el = next;
    };

  }
  free_array(list);
}

int add_edge(struct array *adj_list, long source, long sink) {
  int rc = 0;
  // find source;
  printf("source: %ld\n", source);
  struct list_elem **listp = array_get_safe(adj_list, source);
  struct list_elem *list = *listp;
  // if no existing source in list
  if (list == 0) {
    printf("inserting first edge...\n");

    struct list_elem *el = malloc(sizeof(struct list_elem));
    
    *el = (struct list_elem) {.forward = NULL, .back = NULL, .elem = sink};
    
    array_put_safe(adj_list, source, &el);
  } else {
    for (struct list_elem *el = list; list != NULL; list = list->forward) {
      printf("el: %p\n", el);
    }
  }

  return rc;
}

void *array_get(struct array *arr, size_t index) {
  return arr->data + index * arr->element_size;
}

void *array_get_safe(struct array *arr, size_t index) {
  if (index > arr->capacity) {
    return NULL;
  }
  return array_get(arr, index);
}

void array_put(struct array *arr, size_t index, void *data) {
  memcpy(arr->data + (index * arr->element_size), data, arr->element_size);
}

int array_put_safe(struct array *arr, size_t index, void *data) {
  if (index > arr->capacity) {
    void *temp = realloc(arr->data, (arr->capacity * 2) * arr->element_size);
    if (temp) {
      arr->data = temp;
      arr->capacity = arr->capacity * 2;
    } else {
      return 1;
    }
  }
  array_put(arr, index, data);
  return 0;
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
