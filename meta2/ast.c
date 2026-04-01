#include "ast.h"
#include <stdio.h>
#include <stdlib.h>

const char *node_names[] = {"Program",   "Function", "Parameters", "Parameter",
                            "Arguments", "Integer",  "Double",     "Identifier",
                            "Natural",   "Decimal",  "Call",       "If",
                            "Add",       "Sub",      "Mul",        "Div"};

// create a node of a given category with a given lexical symbol
struct node *newnode(enum category category, char *token) {
  struct node *new = malloc(sizeof(struct node));
  new->category = category;
  new->token = token;
  new->children = malloc(sizeof(struct node_list));
  new->children->node = NULL;
  new->children->next = NULL;
  return new;
}

// append a node to the list of children of the parent node
void addchild(struct node *parent, struct node *child) {
  struct node_list *new = malloc(sizeof(struct node_list));
  new->node = child;
  new->next = NULL;
  struct node_list *children = parent->children;
  while (children->next != NULL)
    children = children->next;
  children->next = new;
}

void show(struct node *node, int depth) {
  if (node == NULL) {
    return;
  }

  for (int i = 0; i < depth; i++) {
    printf("__");
  }

  printf("%s", node_names[node->category]);

  if (node->token != NULL) {
    printf(("(%s)"), node->token);
  }

  printf("\n");

  struct node_list *childrens = node->children;
  while (childrens != NULL) {
    show(childrens->node, depth + 1);
    childrens = childrens->next;
  }
}

struct node_list *newlist(struct node *n) {
  struct node_list *newlist = malloc(sizeof(struct node_list));
  newlist->node = n;
  newlist->next = NULL;
  return newlist;
}

struct node_list *append(struct node_list *list, struct node *n) {
  if (list == NULL) {
    return newlist(n);
  }

  struct node_list *current_node = list;
  while (current_node->next != NULL) {
    current_node = current_node->next;
  }
  current_node->next = newlist(n);
  return list;
};

void addchildren(struct node *parent, struct node_list *list) {
  if (parent != NULL) {
    parent->children = list;
  }
}
