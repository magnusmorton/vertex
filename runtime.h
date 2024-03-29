/**
 * Copyright 2021 University of Edinburgh. All rights reserved.
 * Use of this source code is governed by a BSD-style
 * license that can be found in the LICENSE file.
 */


#include <stdlib.h>


typedef enum Detected {
	LL,
	DOUBLE_LL,
	ARRAY,
	TREE,
	GRAPH,
	MAYBE
} Detected;


struct DataType {
	enum Detected type;
	struct DataType *inner;
	const char *filename;
	unsigned line;
};

#ifdef __cplusplus
extern "C"
#endif
size_t get_detected(Detected**);

#ifdef __cplusplus
extern "C"
#endif
void mark_root(const char *, void *, size_t, const char *, unsigned);

#ifdef __cplusplus
extern "C"
#endif
void check_ptr(void *, const char *, unsigned);

#ifdef __cplusplus
extern "C"
#endif
void handle_store(void *, void *);

#ifdef __cplusplus
extern "C"
#endif
int init_san();

#ifdef __cplusplus
extern "C"
#endif
void finish_san();

#ifdef __cplusplus
extern "C"
#endif
enum Detected detectedDataStructures();
