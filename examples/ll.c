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
  link *a, *b, *c;
  a = malloc(sizeof(link));
  b = malloc(sizeof(link));
  c = malloc(sizeof(link));
  a->data = 1;
  a->next = NULL;
  b->data = 2;
  b->next = NULL;
  c->data = 3;
  c->next = NULL;
  insert_append (a, b);
  insert_append (a, c);
  free(a);
  free(b);
  free(c);
}
