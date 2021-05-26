#include <stdlib.h>

int init_san();
void finish_san();
void _mark_root(const char *, void *, size_t, const char*, unsigned);
void _check_ptr(void *, const char*, unsigned);
