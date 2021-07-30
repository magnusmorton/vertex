#include <stdio.h>
#include <stdlib.h>

typedef struct tree_node {
	struct tree_node *left;
	struct tree_node *right;
	int data;
} node;



node*
node_new(int data, node *left, node *right)
{
	node *tmp = malloc(sizeof(node));
	tmp->data = data;
	tmp->left = left;
	tmp->right = right;
	return tmp;
}

int
main() 
{
  node *a, *b, *c;
  a = node_new(1, NULL, NULL);
  b = node_new(2, NULL, NULL);
  c = node_new(3, a, b);
  free(a);
  free(b);
  free(c);
}
