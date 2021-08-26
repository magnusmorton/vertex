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

#include <algorithm>
#include <iostream>
#include <map>
#include <vector>

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include <igraph/igraph.h>

#include "runtime.h"

#define ROOT_CHUNK 512

struct memory_node {
  char *addr;
  size_t extent;
  std::map<unsigned long, int> slots;

  memory_node(char *a, size_t ex) : addr(a), extent(ex)
  {}
};

static int inited = 0;

igraph_t mem_graph;

std::vector<memory_node> root_nodes;


Detected detect_from_component(igraph_t *subgraph) {
  igraph_vector_t in_degree, out_degree;
  igraph_vector_init(&in_degree, 1);
  igraph_vector_init(&out_degree, 1);

  igraph_vs_t vertices;
  igraph_vs_all(&vertices);

  igraph_integer_t size;
  igraph_vs_size(subgraph, &vertices, &size);
  Detected ret = MAYBE;
  printf("size: %d\n", size);

  for (const auto& vnode : root_nodes) {
    for (const auto& slot_el: vnode.slots) {
      std::cerr << "offset: " << slot_el.first << ", ";
    }
    std::cerr << std::endl;
  }

  /* Detect data structures inefficiently for now. Fuse loops later */
  if (size == 1) {
    ret = ARRAY;
  }
  else {
    int is_tree = 0;
    igraph_is_tree(subgraph, &is_tree, NULL, IGRAPH_ALL);
    if (is_tree) {
      ret = TREE;
    }

    igraph_degree(subgraph, &in_degree, vertices, IGRAPH_IN, 1);
    igraph_degree(subgraph, &out_degree, vertices, IGRAPH_OUT, 1);

    /* try detecting single LL */
    int is_ll = 1;
    for (int i = 0; i < size; i++) {
      int in, out;
      in = VECTOR(in_degree)[i];
      out = VECTOR(out_degree)[i];
      if (in > 1 || out > 1) {
        is_ll = 0;
        break;
      }
    }
    if (is_ll)
      ret = LL;
    int is_dll = 1;
    for (int i = 0; i < size; i++) {
      int in, out;
      in = VECTOR(in_degree)[i];
      out = VECTOR(out_degree)[i];
      printf("in: %d out: %d i: %d\n", in, out, i);
      if (in != 1 || out != 1 ) {
        if ((!out && !i) || (!in && i != size)) {
          is_dll = 0;
          break;
        }
      }
    }
    if (is_dll) {
      ret = DOUBLE_LL;
    }

  }

  igraph_vector_destroy(&in_degree);
  igraph_vector_destroy(&out_degree);
  return ret;
}

size_t get_detected(Detected **out) {

  /**
    separate connected commponents are obviously separate data structures
   **/
  igraph_vector_ptr_t components;
  igraph_vector_ptr_init(&components, 1);

  igraph_decompose(&mem_graph, &components, IGRAPH_WEAK, -1, 1);

  size_t ccs = igraph_vector_ptr_size(&components);

  *out = static_cast<Detected*>(malloc(sizeof(Detected) * ccs));
  for (int i = 0; i < ccs; i++){
    Detected ds_type = detect_from_component(static_cast<igraph_t*>(VECTOR(components)[i]));
    *out[i] = ds_type;
  }
  igraph_decompose_destroy(&components);
  igraph_vector_ptr_destroy(&components);
        
  return ccs;
}

void decode_enum(Detected type, char *str) {
  switch(type) {
    case LL:
      sprintf(str, "LL");
      break;
    case DOUBLE_LL:
      sprintf(str, "DOUBLE LL");
      break;
    case ARRAY:
      sprintf(str, "ARRAY");
      break;
    case GRAPH:
      sprintf(str, "GRAPH");
      break;
    case TREE:
      sprintf(str, "TREE");
      break;
    case MAYBE:
      sprintf(str, "MAYBE");
      break;
  }
}

void finish_san() {
  if (!inited)
    return;
  Detected *detected;
  size_t len = get_detected(&detected);
  fprintf(stderr, "number of datastructures: %lu\n", len);

  char out[16];
  for (int i = 0; i < len; i++) {
    decode_enum(detected[i], out);
    fprintf(stderr, "Data structure %d: %s\n", i, out);
  }
  FILE *f = fopen("graph.dot", "w");
  igraph_write_graph_dot(&mem_graph, f);
  fclose(f);

  free(detected);
  igraph_destroy(&mem_graph);
  inited = 0;
}

int init_san() {
  if (inited)
    return 1;
  fprintf(stderr, "initing runtime....\n");
  igraph_set_attribute_table(&igraph_cattribute_table);
  igraph_empty(&mem_graph, 0, IGRAPH_DIRECTED);
  inited = 1;
  // realisitcally, any errors are going to be unrecoverable here
  return 0;

}


bool match_root(char *addr, memory_node &node) {
  return addr >= node.addr && addr < node.addr + node.extent;
}

void mark_root(const char* label, void *ptr,
    size_t size, const char* file, unsigned line) {
  fprintf(stderr, "ROOT at ptr %p, extent %lu, label %s, file %s:%d\n",
      ptr, size, label, file, line);

  memory_node nd(static_cast<char*>(ptr), size); 
  root_nodes.push_back(nd);;
  igraph_add_vertices(&mem_graph, 1, NULL);

}

void check_ptr(void *ptr, const char *file, unsigned line) {
  auto it = std::find_if(root_nodes.begin(), root_nodes.end(),
                         [=](memory_node n) { return match_root(static_cast<char*>(ptr), n); });
  
  if (it != root_nodes.end()) {
    fprintf(stderr, "ptr %p (%s:%d) belongs to root %p\n",
      ptr, file, line, it->addr);
  }
}

void handle_store(void *vtarget, void *vsource) {
  unsigned long ti, si;
  char *target = static_cast<char*>(vtarget);
  char *source = static_cast<char*>(vsource);
  auto t_found = std::find_if(root_nodes.begin(), root_nodes.end(), [=](memory_node n) { return match_root(target,n);});
  auto s_found = std::find_if(root_nodes.begin(), root_nodes.end(), [=](memory_node n) { return match_root(source,n);});
  
  auto end = std::end(root_nodes);
  if (t_found != end && s_found != end ) {
    ti = t_found - root_nodes.begin();
    si = s_found - root_nodes.begin();
    fprintf(stderr, "handling store..... %p into %p\n", source, target);
    fprintf(stderr, "adding edge from %ld to %ld\n", ti, si);

    memory_node& target_node = *t_found;
    memory_node& stored_node = *s_found;

    long offset = target - target_node.addr;
    fprintf(stderr, "offset: %ld\n", offset);


    /* add edges in reverse order so first alloc is root */
    igraph_add_edge(&mem_graph, ti, si);
    igraph_integer_t eid;
    igraph_get_eid(&mem_graph, &eid, ti, si, IGRAPH_DIRECTED, 0);

		std::cerr << "eid: " << eid << std::endl;

    auto it = target_node.slots.find(offset);
    if (it != target_node.slots.end()) {
      igraph_integer_t prev_store = it->second;
      fprintf(stderr, "deleting edge %d\n", prev_store);
      igraph_delete_edges(&mem_graph, igraph_ess_1(prev_store));
    }
    target_node.slots[offset] = eid;
  }
}
