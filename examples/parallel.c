#include <stdio.h>
#include <stdlib.h>

typedef struct link {
  struct link *one;
  struct link *two;
  int data;
} link;


link *link_new(int data) {
  link *tmp = malloc(sizeof(link));
  tmp->one = NULL;
  tmp->two = NULL;
  tmp->data = data;
  return tmp;
}


void insert_append (link *anchor, link *newlink) {
  newlink->one = anchor->one;
  anchor->one = newlink;
}


int main() {
  link *a, *b, *c;
  a = link_new(1);
  b = link_new(2);
  c = link_new(3);

  insert_append (a, b);
  insert_append (a, c);
  a->two = c;

  free(a);
  free(b);
  free(c);

}
