#include <stdio.h>
#include <stdlib.h>

typedef struct link {
  struct link *next;
  int data;
} link;


void insert_append (link *anchor, link *newlink) {
  newlink->next = anchor->next;
  anchor->next = newlink;
}


int main() {
  link *a, *b, *c, *d, *e, *f;
  a = malloc(sizeof(link));
  b = malloc(sizeof(link));
  c = malloc(sizeof(link));
  d = malloc(sizeof(link));
  e = malloc(sizeof(link));
  f = malloc(sizeof(link));
  a->data = 1;
  b->data = 2;
  c->data = 3;
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
