#include <stdio.h>

struct Node {
	int val;
	Node *next;
};

int main()
{
	Node *node = new Node();
	node->val = 1;
	printf("newd at %p\n", node);
}
	
