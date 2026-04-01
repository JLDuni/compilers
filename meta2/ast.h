#ifndef _AST_H
#define _AST_H

enum category {
  Program,
  Function,
  Parameters,
  Parameter,
  Arguments,
  Integer,
  Double,
  Identifier,
  Natural,
  Decimal,
  Call,
  If,
  Add,
  Sub,
  Mul,
  Div
};

struct node {
  enum category category;
  char *token;
  struct node_list *children;
};

struct node_list {
  struct node *node;
  struct node_list *next;
};

struct node *newnode(enum category category, char *token);
void addchild(struct node *parent, struct node *child);
void show(struct node *node, int depth);
struct node_list *newlist(struct node *n);
struct node_list *append(struct node_list *list, struct node *n);
void addchildren(struct node *parent, struct node_list *list);

#endif
