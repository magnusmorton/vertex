#include <assert.h>
#include <limits.h>
#include <sanitizer/dfsan_interface.h>
#include <stdio.h>
#include <stdlib.h>

#define ROOT_CHUNK 512
#define EDGE_CHUNK 1024

static int labels;
const int base_size = 7;

static dfsan_label *roots;
static size_t root_count = 0;
static size_t roots_size = 0;

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
  dfsan_label *temp = malloc(ROOT_CHUNK * sizeof(dfsan_label));
  int rc = 1;

  if (temp) {
    roots = temp;
    rc = 0;
    roots_size = ROOT_CHUNK;
  }

  return rc;
}

int add_root(dfsan_label label) {
  if (root_count == roots_size) {
    dfsan_label *temp = realloc(roots, (roots_size + ROOT_CHUNK) * sizeof(dfsan_label));
    if (temp) {
      roots = temp;
      roots_size = roots_size + ROOT_CHUNK;
    } else
      return 1;
  }

  roots[root_count++] = label;
  return 0;
}

void _create_label(const char* label, void *ptr, size_t size) {
  dfsan_label lab = dfsan_create_label(label, 0);
  dfsan_set_label(lab, ptr, size);
  printf("ROOT at ptr %p, label %s\n", ptr, label);
  int rc = add_root(lab);
  assert(rc == 0);
}

void _check_ptr(void *ptr) {
  dfsan_label check = dfsan_read_label(ptr, sizeof(ptr));

  for (unsigned long i = 0; i < root_count; i++) {
    if (dfsan_has_label(check, roots[i])) {
//      add_edge();
      printf("ptr belongs to root 0xxxxx lab %d\n", roots[i]);
    }
  }
}

void finish_san() {
  free(roots);
  root_count = 0;
  roots_size = 0;
}
