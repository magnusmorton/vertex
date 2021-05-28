/*
   Data structures and utility functions
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

#ifndef MAG_UTIL_H
#define MAG_UTIL_H

#include <stdlib.h>

struct array {
  size_t capacity, length, element_size;
  void *data;
};

typedef struct array mag_array;

int init_array(mag_array*, size_t, size_t);

void* array_get(mag_array*, size_t);

void array_put(mag_array*, size_t, void*);

int array_push(mag_array*, void*);

void free_array(mag_array*);

mag_array make_adj_list();

int add_edge(mag_array *adj_list, int source, int sink);

void destroy_adj_list(mag_array *adj_list);

#endif
