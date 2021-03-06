#include <sanitizer/dfsan_interface.h>
static int labels;
char label_base[] = "dfslab";

void _create_label(void *ptr, size_t size)
{
	
}
