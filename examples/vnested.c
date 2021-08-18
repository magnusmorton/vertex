#include <stdio.h>
#include <stdlib.h>

typedef struct link {
	struct link *next;
	void *data;
} link;

void
insert_append (link *anchor, link *newlink)
{
	newlink->next = anchor->next;
	anchor->next = newlink;
}

link*
link_new(long data)
{
	link *tmp = malloc(sizeof(link));
	tmp->next = NULL;
	tmp->data = (void *)data;
	return tmp;
}

link*
dlink_new(void *data)
{
	link *tmp = malloc(sizeof(link));
	tmp->next = NULL;
	tmp->data = data;
	return tmp;
}

int 
main()
{
	link **arr = malloc(sizeof(link*) * 16);
	link *a, *b, *c, *d, *e;
	a = link_new(1);
	d = link_new(4);
	arr[0] = a;
	arr[1] = d;
	b = link_new(2);
	c = link_new(3);
	e = link_new(5);
	insert_append(d, e);
	insert_append(a, b);
	insert_append(a, c);
	free(a);
	free(b);
	free(c);
	free(arr);
}
