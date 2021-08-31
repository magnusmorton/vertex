#include <stdio.h>
#include <stdlib.h>

typedef struct link {
  struct link *next;
  struct link *prev;
  int data;
} link;


void
insert_append (link *anchor, link *newlink)
{
  link *next = anchor->next;
  newlink->next = next;
  if (next)
    next->prev = newlink;
  newlink->prev = anchor;
  anchor->next = newlink;
}

link*
link_new(int data)
{
  link *tmp = malloc(sizeof(link));
  tmp->next = NULL;
  tmp->prev = NULL;
  tmp->data = data;
  return tmp;
}

int
main()
{
  link *a, *b, *c;
  a = link_new(1);
  b = link_new(2);
  c = link_new(3);
  insert_append (a, b);
  insert_append (a, c);
  free(a);
  free(b);
  free(c);
}
