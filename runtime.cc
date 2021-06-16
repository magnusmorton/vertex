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
#include <fstream>
#include <iostream>
#include <functional>
#include <vector>

#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graphviz.hpp>

#include <limits.h>
#include <stdlib.h>

#include "runtime.h"


bool inited = false;

struct memory_node {
  char *start;
  char *end; 
};

std::vector<memory_node> roots;

using namespace boost;

typedef adjacency_list<vecS,vecS,directedS> MemGraph;
std::vector<MemGraph::vertex_descriptor> vds;
MemGraph memGraph;

void finish() {
  std::ofstream file;
  file.open("graph.dot");
  write_graphviz(file, memGraph);
  file.close();
}

extern "C" void _mark_root(const char* label, char *ptr, size_t size, const char* file, unsigned line) {
  if (!inited) {
    atexit(&finish);
    inited = true;
  }
  printf("ROOT at ptr %p, extent %lu, label %s, file %s:%d\n", ptr, size, label, file, line);
  struct memory_node nd = {ptr, ptr + size};
  roots.push_back(nd);
  vds.push_back(add_vertex(memGraph));
}

extern "C" void _check_ptr(char *ptr, const char *file, unsigned line) {
  std::cerr << "doing nothing\n";
}

std::function<bool(memory_node&)> find_func(char *ptr) {
  return [ptr](memory_node &node) { return ptr >= node.start && ptr <= node.end; };
}

extern "C" void _handle_store(char *target, char *source) {
  printf("handling store.....\n");
  auto ti = std::find_if(roots.begin(), roots.end(), find_func(target));
  auto si = std::find_if(roots.begin(), roots.end(), find_func(source));

  if (ti == roots.end() || si == roots.end())
    std::cerr << "target or source not found\n";
  else {
    printf("source %ld, sink %ld\n", si-roots.begin(), ti-roots.begin());
    auto svd = vds[si-roots.begin()];
    auto tvd = vds[ti-roots.begin()];
    add_edge(svd, tvd, memGraph);
  }
}


