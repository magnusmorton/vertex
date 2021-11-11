// Copyright 2021 University of Edinburgh. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/connected_components.hpp>
#include <boost/graph/copy.hpp>
#include <boost/graph/graphviz.hpp>

#include "runtime.h"

#define ROOT_CHUNK 512

// custom graph properties

struct vertex_property {
  int component;
  bool has_substructure = false;
};

struct edge_property {
};

struct graph_property {
  unsigned number_of_components;
};

// The main memory graph type
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS,
                              vertex_property> MemGraph;

// An undirected graph we use for weakly connected components
typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS,
                              vertex_property> UGraph;

std::vector<MemGraph::vertex_descriptor> vds;
MemGraph graph;

unsigned max_offset = 0;

// An allocation location
struct location {
  std::string file;
  unsigned line;
};


// Represents an allocated memory location
struct memory_node {
  unsigned counter;
  char *addr;
  size_t extent;
  location where_defined;
  // slots holds the offsets to the base address in memory where other pointers
  // are stored
  std::map<unsigned long, std::optional<MemGraph::edge_descriptor>> slots;

  // Update counter everytime we construct a memory_node
  memory_node(char *a, size_t ex, location loc) : addr(a), extent(ex), where_defined(loc)
  {
    counter = total_count++;
  }  

  // needs to be called again if slots is updated
  unsigned compute_code();

  // A code to distinguish between different data-structures of the same type
  unsigned code() {
    if (!calculated) {
      compute_code();
      calculated = true;
    }
    return _code;
  } 
private:
  static unsigned total_count;
  unsigned _code = 0;
  bool calculated = false;
};

unsigned memory_node::total_count = 0;

// Calculate unique(?) code based on the memory addresses of pointers stored
// in this block of memory
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

// DataType constructor. All pointer arguments can be nullptr
DataType* make_datatype(Detected d, DataType *subtype = nullptr, const char *file=nullptr, unsigned line = 0) {
  DataType *out = static_cast<DataType*>(malloc(sizeof(DataType)));
  out->type = d;
  out->inner = subtype;
  out->filename = file;
  out->line = line;
  return out;
}

// Having separate maps/vectors for everything is much easier than storing all
// this stuff in a boost::graph

// vector of memory_nodes detected on allocation
std::vector<std::shared_ptr<memory_node>> root_nodes;

// Holds a map of edges that are removed during component splitting
std::multimap<MemGraph::vertex_descriptor, MemGraph::vertex_descriptor> removed_edges;

// A map containing the first nodes allocated that belong to a component
std::map<unsigned, std::shared_ptr<memory_node>> component_original_nodes;


// Detects a single non-nested data type from a graph component
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

// Calculate weakly connected components
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

// Get detected data structures
size_t get_detected(DataType ***out) {

  std::vector<MemGraph::edge_descriptor> del_edges;
  for (auto [eit, e_end] = boost::edges(graph); eit != e_end; ++eit) {
    MemGraph::vertex_descriptor vt = boost::target(*eit, graph) ;
    MemGraph::vertex_descriptor vs = boost::source(*eit, graph) ;
    std::shared_ptr<memory_node> nt = root_nodes[vt];
    std::shared_ptr<memory_node> ns = root_nodes[vs];
    
    if (nt->code() != ns->code()) {

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
    std::shared_ptr<memory_node> node = root_nodes[vd];
    auto it = component_original_nodes.find(component);

    // Update the original node for the component if a) one doesn't exist or b)
    // node is older than the one currently stored for the component.
    if (it == component_original_nodes.end() || node->counter < it->second->counter)
        component_original_nodes[component] = node;

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
    unsigned i = it - components.begin();
    Detected ds_type = detect_from_subtype(*it);
    component_types[i] = ds_type;
  }
  int i = 0;
  for (unsigned comp : root_components) {
    unsigned curr = comp;
    bool end = false;
    memory_node& original = *component_original_nodes[curr];
    DataType *top = make_datatype(component_types[curr], nullptr, original.where_defined.file.c_str(), original.where_defined.line);
    DataType *parent = top;
    while (!end) {
      curr = component_map[curr];
      if (curr == -1) {
        end = true;
        break;
      } else {
        parent->inner = make_datatype(component_types[curr]);
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
    if (curr->line) {
      out << "(" << curr->filename << ":" << curr->line << ") ";

    }
    decode_enum(curr->type, out);
    
    if (curr->inner) {
      out << " of" << std::endl << "\t";
    }
    curr = curr->inner;
  }
  out << std::endl;
}


// function run on shutdown
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


bool match_root(char *addr, memory_node& node) {
  return addr >= node.addr && addr < node.addr + node.extent;
}


// mark allocation
void mark_root(const char* label, void *ptr,
    size_t size, const char* file, unsigned line) {
  location loc = {file, line};
  root_nodes.push_back(std::make_shared<memory_node>(static_cast<char*>(ptr), size, loc));;
  vds.push_back(boost::add_vertex(graph));

}

void check_ptr(void *ptr, const char *file, unsigned line) {
  auto it = std::find_if(root_nodes.begin(), root_nodes.end(),
                         [=](std::shared_ptr<memory_node> n) { return match_root(static_cast<char*>(ptr), *n); });
  
  if (it != root_nodes.end()) {
  }
}


// handle store instructions
void handle_store(void *vtarget, void *vsource) {
  unsigned long ti, si;
  char *target = static_cast<char*>(vtarget);
  char *source = static_cast<char*>(vsource);
  auto t_found = std::find_if(root_nodes.begin(), root_nodes.end(), [=](std::shared_ptr<memory_node> n) { return match_root(target, *n);});
  auto s_found = std::find_if(root_nodes.begin(), root_nodes.end(), [=](std::shared_ptr<memory_node> n) { return match_root(source, *n);});
  
  auto end = std::end(root_nodes);
  if (t_found != end) {
    ti = t_found - root_nodes.begin();
    std::shared_ptr<memory_node> target_node = *t_found;
    long offset = target - target_node->addr;
    if (offset > max_offset)
      max_offset = offset;

    if (s_found != end) {
      si = s_found - root_nodes.begin();

      /* add edges in reverse order so first alloc is root */
      auto[edge, added] = boost::add_edge(vds[ti], vds[si], graph);

      auto it = target_node->slots.find(offset);
      if (it != target_node->slots.end()) {
      
        MemGraph::edge_descriptor prev_store;
        if (it->second.has_value()) {
          prev_store = it->second.value();
          boost::remove_edge(prev_store, graph);
        }
      }
      target_node->slots[offset] = edge;
    }
    else if (!vsource) {
      target_node->slots[offset] = {};
    }
  }
}
