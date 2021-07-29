#include <stdlib.h>

#include <gmodule.h>

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
};


GArray* get_detected();

void mark_root(const char *, void *, size_t, const char *, unsigned);

void check_ptr(void *, const char *, unsigned);

void handle_store(void *, void *);

int init_san();
void finish_san();

enum Detected detectedDataStructures();
