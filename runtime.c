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


Detected
detect_from_component(igraph_t *subgraph)
{
	igraph_vector_t in_degree, out_degree;
	igraph_vector_init(&in_degree, 1);
	igraph_vector_init(&out_degree, 1);

	igraph_vs_t vertices;
	igraph_vs_all(&vertices);

	igraph_integer_t size;
	igraph_vs_size(subgraph, &vertices, &size);
	Detected ret = MAYBE;
	printf("size: %d\n", size);

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

	GArray *structures = g_array_sized_new(FALSE, FALSE,
					       sizeof(Detected), ccs);

	for (int i = 0; i < ccs; i++){
		Detected ds_type = detect_from_component(VECTOR(components)[i]);
		g_array_append_val(structures, ds_type);
	}
	igraph_decompose_destroy(&components);
	igraph_vector_ptr_destroy(&components);

	return structures;
}

void
decode_enum(Detected type, char *str)
{
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

void
finish_san()
{
	if (!inited)
		return;
	GArray *detected = get_detected();
	fprintf(stderr, "number of datastructures: %d\n", detected->len);
	
	char out[16];
	for (int i = 0; i < detected->len; i++) {
		decode_enum(g_array_index(detected, Detected,i), out);
		fprintf(stderr, "Data structure %d: %s\n", i, out);
	}
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
	igraph_i_set_attribute_table(&igraph_cattribute_table);
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
		
		/* add edges in reverse order so first alloc is root */
		igraph_add_edge(&mem_graph, ti, si);
		igraph_integer_t eid;
		igraph_get_eid(&mem_graph, &eid, ti, si, IGRAPH_DIRECTED, FALSE);

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
