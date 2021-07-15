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

#include <gmodule.h>
#include <igraph/igraph.h>

#include "runtime.h"

#define ROOT_CHUNK 512

struct memory_node {
	void *addr;
	size_t extent;
  
};

static int inited = 0;

igraph_t mem_graph;

GHashTable *prev_stores;
GArray *root_nodes;

GArray*
get_detected()
{

	/**
	   separate connected commponents are obviously separate data structures
	**/
	igraph_vector_ptr_t components;
	igraph_vector_ptr_init(&components, 1);

	igraph_decompose(&mem_graph, &components, IGRAPH_WEAK, -1, 1);

	size_t ccs = igraph_vector_ptr_size(&components);
  
	igraph_decompose_destroy(&components);
	igraph_vector_ptr_destroy(&components);

	Detected maybe = MAYBE;
	GArray *structures = g_array_sized_new(FALSE, FALSE,
					       sizeof(Detected), ccs);
	for (size_t i = 0; i < ccs; i++)
		g_array_append_val(structures, maybe);
	return structures;
}

void
finish_san()
{
	if (!inited)
		return;
	GArray *detected = get_detected();
	fprintf(stderr, "number of datastructures: %d\n", detected->len);

	FILE *f = fopen("graph.dot", "w");
	igraph_write_graph_dot(&mem_graph, f);
	fclose(f);

	g_array_free(detected, TRUE);
	g_array_free(root_nodes, TRUE);
	g_hash_table_destroy(prev_stores);  
	igraph_destroy(&mem_graph);
	inited = 0;
}

int
init_san()
{
	if (inited)
		return 1;
	fprintf(stderr, "initing runtime....\n");
	root_nodes = g_array_sized_new(FALSE, FALSE, sizeof(struct memory_node),
				       ROOT_CHUNK);
	igraph_empty(&mem_graph, 0, IGRAPH_DIRECTED);
	prev_stores = g_hash_table_new(NULL, NULL);
	inited = 1;
	// realisitcally, any errors are going to be unrecoverable here
	return 0;

}

int
search_roots(void *addr, unsigned long *index)
{
	for (size_t i = 0; i < root_nodes->len; i++) {
		struct memory_node *mem = &g_array_index(root_nodes,
							 struct memory_node, i);
		if (addr >= mem->addr && addr < mem->addr + mem->extent ) {
			*index = i;
			return 1;
		}
	}
	return 0;
}

void
mark_root(const char* label, void *ptr,
	  size_t size, const char* file, unsigned line)
{
	fprintf(stderr, "ROOT at ptr %p, extent %lu, label %s, file %s:%d\n",
		ptr, size, label, file, line);
	struct memory_node nd = {.addr = ptr, .extent = size};
	g_array_append_val(root_nodes, nd);
	igraph_add_vertices(&mem_graph, 1, NULL);

}

void
check_ptr(void *ptr, const char *file, unsigned line)
{
	unsigned count = 0;
	for (unsigned long i = 0; i < root_nodes->len; i++) {
		struct memory_node *nd = &g_array_index(root_nodes,
							struct memory_node, i);
		if (ptr >= nd->addr && ptr < nd->addr + nd->extent) {

			fprintf(stderr, "ptr %p (%s:%d) belongs to root %p\n",
				ptr, file, line, nd->addr);
			count++;
		}
	}
	if (count > 1)
		fprintf(stderr, "\tprobable indirection\n"); 
}

void
handle_store(void *target, void *source)
{
	unsigned long ti, si;
	int t_found = search_roots(target, &ti);
	int s_found = search_roots(source, &si);

	if (t_found && s_found ) {
		fprintf(stderr, "handling store..... %p\n", target);
		fprintf(stderr, "adding edge from %ld to %ld\n", si, ti);

		igraph_add_edge(&mem_graph, si, ti);
		igraph_integer_t eid;
		igraph_get_eid(&mem_graph, &eid, si, ti, IGRAPH_DIRECTED, FALSE);

		gpointer ret;
		if (g_hash_table_lookup_extended(
			    prev_stores, target, NULL, &ret)) {
			gint prev_edge = GPOINTER_TO_INT(ret);
			fprintf(stderr, "deleting edge %d\n", prev_edge);
			igraph_delete_edges(&mem_graph, igraph_ess_1(prev_edge));
		}
		g_hash_table_insert(prev_stores,  target, GINT_TO_POINTER(eid));
	}
}
