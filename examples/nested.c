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
  link *a, *b, *c, *d, *e, *out_a, *out_b;
  a = link_new(1);
  b = link_new(2);
  c = link_new(3);
  d = link_new(4);
  e = link_new(5);
  insert_append(d, e);
  insert_append(a, b);
  insert_append(a, c);
  out_a = dlink_new(a);
  out_b = dlink_new(d);
  insert_append(out_a, out_b);
  free(a);
  free(b);
  free(c);
}
