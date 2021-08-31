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
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/graphviz.hpp>


#include "runtime.h"

#define ROOT_CHUNK 512

struct vertex_property {
  int component;
};

struct edge_property {
};

struct graph_property {
  unsigned number_of_components;
};

static int inited = 0;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                              vertex_property> MemGraph;
std::vector<MemGraph::vertex_descriptor> vds;
MemGraph _graph;

struct memory_node {
  char *addr;
  size_t extent;
  std::map<unsigned long, int> slots;
  std::map<unsigned long, MemGraph::edge_descriptor> _slots;
  memory_node(char *a, size_t ex) : addr(a), extent(ex)
  {}
};

std::vector<memory_node> root_nodes;


Detected detect_from_component(std::vector<MemGraph::vertex_descriptor> &subgraph) {
  Detected ret = MAYBE;
  
  std::cerr << "size: " << subgraph.size() << std::endl;
  
  // TODO: something slots
  
  if (subgraph.size() == 1) {
    ret = ARRAY;
  }
  else {
    // TODO: make is_tree. or do we care?
    ret = TREE;

    // single LL
    bool is_ll = true;
    for (auto vd : subgraph) {
      unsigned in = boost::in_degree(vd, _graph);
      unsigned out = boost::out_degree(vd, _graph);

      std::cerr << "in: " << in << " out " << out << " i: " << vd << std::endl;
      if (in > 1 || out > 1) {
        is_ll = false;
        break;
      }
    }
    if (is_ll)
      ret = LL;

    bool is_dll = true;
    // TODO: fuse with above loop
    for (unsigned i = 0; i < subgraph.size(); ++i) {
      MemGraph::vertex_descriptor vd = subgraph[i];
      unsigned in = boost::in_degree(vd, _graph);
      unsigned out = boost::out_degree(vd, _graph);
      if (in != 2 || out != 2) {
        std::cerr << "not 2" << std::endl;
        if (in != 1 || out != 1) {
          if (i != 0 || i != subgraph.size()) {
            is_dll = false;
            break;
          }
        }
      }
    }
    if (is_dll)
      ret = DOUBLE_LL;
  }
  return ret;
}

unsigned weak_components(MemGraph &graph) {
  std::cerr << "WEAK COMPONENTS" << std::endl;
  // connected components seems to work as is for bidirectionalS graph
  unsigned number_of_components = 
      boost::connected_components(graph, 
                               boost::get(&vertex_property::component, graph));

  return number_of_components;
}



size_t get_detected(Detected **out) {

  /**
    separate connected commponents are obviously separate data structures
   **/

  unsigned num_components = weak_components(_graph);
  
  std::vector<std::vector<MemGraph::vertex_descriptor>> components(num_components);
  for (auto vd : boost::make_iterator_range(vertices(_graph))) {
    components[_graph[vd].component].push_back(vd);
  }

  *out = static_cast<Detected*>(malloc(sizeof(Detected) * num_components));
  for (auto it = components.begin(); it != components.end(); ++it){
    Detected ds_type = detect_from_component(*it);
    *out[it - components.begin()] = ds_type;
  }

  return num_components;
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

  std::ofstream file;
  file.open("graph.dot");
  boost::write_graphviz(file, _graph);
  file.close();

  free(detected);
  inited = 0;
}

int init_san() {
  if (inited)
    return 1;
  fprintf(stderr, "initing runtime....\n");

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
  vds.push_back(boost::add_vertex(_graph));

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

    long offset = target - target_node.addr;
    fprintf(stderr, "offset: %ld\n", offset);


    /* add edges in reverse order so first alloc is root */
    auto[edge, added] = boost::add_edge(vds[ti], vds[si], _graph);

    auto _it = target_node._slots.find(offset);
    if (_it != target_node._slots.end()) {
      MemGraph::edge_descriptor prev_store = _it->second;
      std::cerr << "deleting bgl edge: " << prev_store << std::endl;
      boost::remove_edge(prev_store, _graph);
    }
    target_node._slots[offset] = edge;
  }
}
