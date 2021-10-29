// Copyright 2021 University of Edinburgh. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <optional>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/graphviz.hpp>

#include "runtime.h"

#define ROOT_CHUNK 512

struct vertex_property {
  int component;
  bool has_substructure = false;
};

struct component_vertex {
  Detected type;
};

struct edge_property {
};

struct graph_property {
  unsigned number_of_components;
};

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                              vertex_property> MemGraph;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                              vertex_property> UGraph;

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
                              component_vertex> ComponentGraph;
std::vector<MemGraph::vertex_descriptor> vds;
MemGraph graph;
ComponentGraph structures;

unsigned max_offset = 0;

struct location {
  std::string file;
  unsigned line;
};

struct memory_node {
  char *addr;
  size_t extent;
  location &where_defined;
  std::map<unsigned long, std::optional<MemGraph::edge_descriptor>> slots;
  memory_node(char *a, size_t ex, location& loc) : addr(a), extent(ex), where_defined(loc)
  {}

  // needs to be called again if slots is updated
  unsigned compute_code();
  unsigned code() {
    if (!calculated) {
      compute_code();
      calculated = true;
    }
    return _code;
  } 
private:
  unsigned _code = 0;
  bool calculated = false;
};

unsigned memory_node::compute_code() {
  unsigned acc = 0;
  for(auto it = slots.begin(); it != slots.end(); ++it) {
    if (it == slots.begin()) {
      acc = it->first;
    }
    // pairing function. each pair < max_offset should produce unique sequence
    else {
      acc = acc * max_offset + it->first;
    }
  }
  calculated = true;
  _code = acc;
  return _code;
}



std::ostream& operator<<(std::ostream& os, memory_node& obj) {
  os << "Addr: " << obj.addr << " extent: " << obj.extent << " slots: " << obj.slots.size(); 
  return os;
}

DataType* make_datatype(Detected d, DataType *subtype) {
  DataType *out = static_cast<DataType*>(malloc(sizeof(DataType)));
  out->type = d;
  out->inner = subtype;
  return out;
}

std::vector<memory_node> root_nodes;
std::multimap<MemGraph::vertex_descriptor, MemGraph::vertex_descriptor> removed_edges;


Detected detect_from_subtype(std::vector<MemGraph::vertex_descriptor> &subgraph) {
  Detected ret = MAYBE;
  if (subgraph.size() == 1) {
    ret = ARRAY;
  }
  else {
    // TODO: make is_tree. or do we care?
    ret = TREE;

    // single LL
    bool is_ll = true;
    for (auto vd : subgraph) {
      unsigned in = boost::in_degree(vd, graph);
      unsigned out = boost::out_degree(vd, graph);

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
      unsigned in = boost::in_degree(vd, graph);
      unsigned out = boost::out_degree(vd, graph);
      if (in != 2 || out != 2) {
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

/* Detected detect_all(unsigned component, const std::vector<long> &compenent_map, const std::map<unsigned, Detected> &component_types) { */
/*   long child = component_map; */
/* } */

unsigned weak_components(MemGraph &graph) {
  UGraph copy;
  
  boost::copy_graph(graph, copy);

  // HACK: do connected components on undirected copy and write to properties of
  // directed graph. This will probably break in the future
  unsigned number_of_components = 
      boost::connected_components(copy, 
                               boost::get(&vertex_property::component, graph));
  
  return number_of_components;
}



size_t get_detected(DataType ***out) {

  std::vector<MemGraph::edge_descriptor> del_edges;
  for (auto [eit, e_end] = boost::edges(graph); eit != e_end; ++eit) {
    MemGraph::vertex_descriptor vt = boost::target(*eit, graph) ;
    MemGraph::vertex_descriptor vs = boost::source(*eit, graph) ;
    memory_node& nt = root_nodes[vt];
    memory_node& ns = root_nodes[vs];
    
    if (nt.code() != ns.code()) {

      removed_edges.insert(std::make_pair(vs, vt));
      del_edges.push_back(*eit);
      boost::put(&vertex_property::has_substructure, graph, vs, true);
    }
  }

  for (auto e : del_edges) {
    boost::remove_edge(e, graph);
  }
  

  unsigned num_components = weak_components(graph);
  std::vector<long> component_map(num_components, -1);
  std::map<unsigned, Detected> component_types;
  std::vector<std::vector<MemGraph::vertex_descriptor>> components(num_components);
  std::set<unsigned> root_components;
  std::set<unsigned> seen;
  for (int i = 0; i < num_components; ++i) {
    root_components.insert(i);
  }
  
  for (auto vd : boost::make_iterator_range(vertices(graph))) {
    unsigned component = graph[vd].component;
    components[component].push_back(vd);
    // if vertex is at edge of component, store
    if (boost::get(&vertex_property::has_substructure, graph, vd)) {
      // we need to extract the components which no component points to 
      for (auto[it, end_it] = removed_edges.equal_range(vd); it != end_it; ++it) {
        MemGraph::vertex_descriptor vother = it->second;
        unsigned child_component = boost::get(&vertex_property::component, graph, vother);
        root_components.erase(child_component);
        component_map[component] = child_component;
      }
    } 
  }

  *out = static_cast<DataType**>(malloc(sizeof(DataType*) * root_components.size()));
  for (auto it = components.begin(); it != components.end(); ++it){
    ComponentGraph::vertex_descriptor vd = boost::add_vertex(structures);
    unsigned i = it - components.begin();
    assert(vd == i);
    Detected ds_type = detect_from_subtype(*it);
    component_types[i] = ds_type;
  }
  int i = 0;
  for (unsigned comp : root_components) {
    unsigned curr = comp;
    bool end = false;
    DataType *top = make_datatype(component_types[curr], NULL);
    DataType *parent = top;
    while (!end) {
      
      // TODO: badness is here most likely
      curr = component_map[curr];
      if (curr == -1) {
        end = true;
        break;
      } else {
        parent->inner = make_datatype(component_types[curr], NULL);
        parent = parent->inner;
      }
    }
    (*out)[i++] = top;
  }

  return root_components.size();
}

void decode_enum(Detected type, std::ostream& out) {
  switch(type) {
    case LL:
      out << "LL";
      break;
    case DOUBLE_LL:
      out << "DOUBLE LL";
      break;
    case ARRAY:
      out << "ARRAY";
      break;
    case GRAPH:
      out << "GRAPH";
      break;
    case TREE:
      out << "TREE";
      break;
    case MAYBE:
      out << "MAYBE";
      break;
  }
}

void decode_datatype(DataType *dt, std::ostream& out) {
  DataType *curr = dt;
  while (curr) {
    decode_enum(curr->type, out);
    if (curr->inner) {
      out << " of" << std::endl << "\t";
    }
    curr = curr->inner;
  }
  out << std::endl;
}

void finish_san() {
  std::ofstream file;
  file.open("graph.dot");
  boost::write_graphviz(file, graph);
  file.close();

  DataType **detected;
  size_t len = get_detected(&detected);
  fprintf(stderr, "number of datastructures: %lu\n", len);

  for (int i = 0; i < len; i++) {
    DataType *dt = detected[i];
    decode_datatype(dt, std::cout);
  }

  
  file.open("disconnected.dot");
  boost::write_graphviz(file, graph);
  file.close();
  free(detected);
}


bool match_root(char *addr, memory_node &node) {
  return addr >= node.addr && addr < node.addr + node.extent;
}

void mark_root(const char* label, void *ptr,
    size_t size, const char* file, unsigned line) {
  location loc = {file, line};
  memory_node nd(static_cast<char*>(ptr), size, loc); 
  root_nodes.push_back(nd);;
  vds.push_back(boost::add_vertex(graph));

}

void check_ptr(void *ptr, const char *file, unsigned line) {
  auto it = std::find_if(root_nodes.begin(), root_nodes.end(),
                         [=](memory_node n) { return match_root(static_cast<char*>(ptr), n); });
  
  if (it != root_nodes.end()) {
  }
}

void handle_store(void *vtarget, void *vsource) {
  unsigned long ti, si;
  char *target = static_cast<char*>(vtarget);
  char *source = static_cast<char*>(vsource);
  auto t_found = std::find_if(root_nodes.begin(), root_nodes.end(), [=](memory_node n) { return match_root(target,n);});
  auto s_found = std::find_if(root_nodes.begin(), root_nodes.end(), [=](memory_node n) { return match_root(source,n);});
  
  auto end = std::end(root_nodes);
  if (t_found != end) {
    ti = t_found - root_nodes.begin();
    memory_node& target_node = *t_found;
    long offset = target - target_node.addr;
    if (offset > max_offset)
      max_offset = offset;

    if (s_found != end) {
      si = s_found - root_nodes.begin();

      /* add edges in reverse order so first alloc is root */
      auto[edge, added] = boost::add_edge(vds[ti], vds[si], graph);

      auto it = target_node.slots.find(offset);
      if (it != target_node.slots.end()) {
      
        MemGraph::edge_descriptor prev_store;
        if (it->second.has_value()) {
          prev_store = it->second.value();
          boost::remove_edge(prev_store, graph);
        }
      }
      target_node.slots[offset] = edge;
    }
    else if (!vsource) {
      target_node.slots[offset] = {};
    }
  }
}
