#include <stdio.h>
#include <stdlib.h>

typedef struct link {
  struct link *next;
  int data;
} link;


link *link_new(int data) {
  link *tmp = malloc(sizeof(link));
  tmp->next = NULL;
  tmp->data = data;
  return tmp;
}


void insert_append (link *anchor, link *newlink) {
  newlink->next = anchor->next;
  anchor->next = newlink;
}


int main() {
  link *a, *b, *c, *d, *e, *f;
  a = link_new(1);
  b = link_new(2);
  c = link_new(3);
  d = link_new(4);
  e = link_new(5);
  f = link_new(6);
  insert_append (a, b);
  insert_append (a, c);
  insert_append (d, e);
  insert_append (e, f);
  free(a);
  free(b);
  free(c);
  free(d);
  free(e);
  free(f);
}
