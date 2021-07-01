/*
   memory tracking runtime
   Copyright (C) 2021 University of Edinburgh

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
    
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <igraph/igraph.h>

#include "runtime.h"
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

mag_array adj_list;

igraph_t mem_graph;

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
  adj_list = make_adj_list();
  igraph_empty(&mem_graph, 0, IGRAPH_DIRECTED);

  inited = 1;
  atexit(&finish_san);
  // realisitically, any errors are going to be unrecoverable here
  return 0;

}

long search_roots(void *addr) {
  for (size_t i = 0; i < root_nodes.length; i++) {
    struct memory_node *mem = array_get(&root_nodes, i);
    if (addr >= mem->addr && addr < mem->addr + mem->extent )
      return i;
  }
  return -1;
}

void _mark_root(const char* label, void *ptr, size_t size, const char* file, unsigned line) {
  if (!inited)
    init_san();
  
  printf("ROOT at ptr %p, extent %lu, label %s, file %s:%d\n", ptr, size, label, file, line);
  struct memory_node nd = {.addr = ptr, .extent = size};
  array_push(&root_nodes, &nd);

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

void _handle_store(void *target, void *source) {
  printf("handling store.....\n");
  long ti = search_roots(target);
  long si = search_roots(source);
  add_edge(&adj_list, si, ti);
}

void finish_san() {
  free_array(&root_nodes);
  destroy_adj_list(&adj_list);
  igraph_destroy(&mem_graph);
}
