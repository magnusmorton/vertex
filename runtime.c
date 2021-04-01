#include <assert.h>
#include <limits.h>
#include <sanitizer/dfsan_interface.h>
#include <stdio.h>
#include <stdlib.h>


#include "util.h"

#define ROOT_CHUNK 512
#define EDGE_CHUNK 1024

const int base_size = 7;

static int inited = 0;

mag_array roots;

mag_array root_nodes;


struct memory_node {
  void *addr;
  size_t extent;
  
};

struct edge {
  struct memory_node *a;
  struct memory_node *b;
};

static struct edge *edgelist;
static size_t edge_count = 0;
static size_t edgelist_size = 0;



int numPlaces (int n) {
  if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
  if (n < 10) return 1;
  if (n < 100) return 2;
  if (n < 1000) return 3;
  if (n < 10000) return 4;
  if (n < 100000) return 5;
  if (n < 1000000) return 6;
  if (n < 10000000) return 7;
  if (n < 100000000) return 8;
  if (n < 1000000000) return 9;
  return 10;
}

int init_san() {
  printf("initing runtime....\n");
  init_array(&root_nodes, ROOT_CHUNK, sizeof(struct memory_node));
  init_array(&roots, ROOT_CHUNK, sizeof(dfsan_label));

  inited = 1;
  // realisitically, any errors are going to be unrecoverable here
  return 0;

}

void _create_label(const char* label, void *ptr, size_t size) {
  if (!inited)
    init_san();
  
  dfsan_label lab = dfsan_create_label(label, 0);
  dfsan_set_label(lab, ptr, size);
  printf("ROOT at ptr %p, label %s\n", ptr, label);
  int rc = array_push(&roots, &lab);
  struct memory_node nd = {.addr = ptr, .extent = size};
  array_push(&root_nodes, &nd);
  assert(root_nodes.length == roots.length);
}

void _check_ptr(void *ptr) {
  dfsan_label check = dfsan_read_label(ptr, sizeof(ptr));

  if (!check) {
    printf("label not found\n");
    return;
  }
  
  for (unsigned long i = 0; i < roots.length; i++) {
    dfsan_label lab = *(dfsan_label*)array_get(&roots, i);
    if (dfsan_has_label(check, lab)) {
//      add_edge();

      struct memory_node *nd = (struct memory_node*)array_get(&root_nodes, i);
      printf("ptr %p belongs to root %p lab %d\n", ptr, nd->addr, lab);
      break;
    }
  }
}

void finish_san() {
  free_array(&roots);
  free_array(&root_nodes);
}
