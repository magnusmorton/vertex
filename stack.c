#include <stdio.h>

struct Node {
	int val;
	struct Node *next;
};

int main()
{
	struct Node n;
	n.val = 97;
	n.next = NULL;
	printf("node val %d\n", n.val);
	return 0;
}
