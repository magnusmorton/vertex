#include <assert.h>
#include <limits.h>
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

  // TODO: use a hash table. or test perf
  inited = 1;
  // realisitically, any errors are going to be unrecoverable here
  return 0;

}

void _create_label(const char* label, void *ptr, size_t size, const char* file, unsigned line) {
  if (!inited)
    init_san();
  
  printf("ROOT at ptr %p, extent %lu, label %s, file %s:%d\n", ptr, size, label, file, line);
  struct memory_node nd = {.addr = ptr, .extent = size};
  array_push(&root_nodes, &nd);
  array_push(&roots, ptr);
  assert(root_nodes.length == roots.length);
}

void _check_ptr(void *ptr, const char *file, unsigned line) {
  unsigned count = 0;
  for (unsigned long i = 0; i < root_nodes.length; i++) {
    struct memory_node *nd = array_get(&root_nodes, i);
    if (ptr >= nd->addr && ptr < nd->addr + nd->extent) {

      printf("ptr %p (%s:%d) belongs to root %p\n", ptr, file, line, nd->addr);
      count++;
    }
  }
  printf("count: %d\n", count);
  if (count > 1)
    printf("\tprobable indirection\n"); 
}

void finish_san() {
  free_array(&root_nodes);
}
