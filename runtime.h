#include <stdlib.h>
#ifdef __cplusplus
extern "C"
#endif
void _mark_root(const char *, char*, size_t, const char*, unsigned);

#ifdef __cplusplus
extern "C"
#endif
void _check_ptr(char *, const char*, unsigned);

#ifdef __cplusplus
extern "C"
#endif
void _handle_store(char *target, char *source);
