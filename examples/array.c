#include <stdio.h>
#include <stdlib.h>

int
main()
{
	int* foo = malloc(1024 * sizeof(int));
	for (int i = 0; i < 1024; i++)
		foo[i] = i;
	
	return 0;
}
