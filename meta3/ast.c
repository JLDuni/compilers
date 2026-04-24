#include "ast.h"
#include "semantics.h"
#include <stdio.h>
#include <stdlib.h>

const char *node_names[] = {
    "Program",      "FieldDecl", "VarDecl",    "MethodDecl", "MethodHeader",
    "MethodParams", "ParamDecl", "MethodBody", "Block",      "If",
    "While",        "Return",    "Print",      "Assign",     "Or",
    "And",          "Eq",        "Ne",         "Lt",         "Gt",
    "Le",           "Ge",        "Add",        "Sub",        "Mul",
    "Div",          "Mod",       "Lshift",     "Rshift",     "Xor",
    "Not",          "Minus",     "Plus",       "Length",     "Call",
    "ParseArgs",    "Bool",      "BoolLit",    "Double",     "Decimal",
    "Identifier",   "Int",       "Natural",    "StrLit",     "StringArray",
    "Void"};

// create a node of a given category with a given lexical symbol
struct node *newnode(category category, char *token, int line, int column) {
  struct node *new = malloc(sizeof(struct node));
  if (new == NULL)
    return new;

  new->category = category;
  new->token = token;
  new->children = malloc(sizeof(struct node_list));
  if (new->children == NULL)
    return new;

  new->line = line;
  new->column = column;

  new->parameter_types_str = NULL;
  new->type = no_type;
  new->is_duplicate = 0;
  new->local_symbols = NULL;

  new->children->node = NULL;
  new->children->next = NULL;
  return new;
}

// append a node to the list of children of the parent node
void addchild(struct node *parent, struct node *child) {
  struct node_list *new = malloc(sizeof(struct node_list));
  if (new == NULL)
    return;
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

  extern int annotated_ast;

  for (int i = 0; i < depth; i++) {
    printf("..");
  }

  printf("%s", node_names[node->category]);

  if (node->token != NULL) {
    if (node->category == StrLit) {
      printf("(\"%s\")", node->token);
    } else {
      printf("(%s)", node->token);
    }
  }

  if (annotated_ast && node->parameter_types_str != NULL) {
    printf(" - %s", node->parameter_types_str);
  } else if (annotated_ast && node->type != no_type) {
    printf(" - %s", type_to_string(node->type));
  }

  printf("\n");

  struct node_list *childrens = node->children;
  while (childrens != NULL) {
    show(childrens->node, depth + 1);
    childrens = childrens->next;
  }
}

struct node *getchild(struct node *parent, int position) {
  if (parent == NULL) {
    return NULL;
  }

  struct node_list *children = parent->children;
  while ((children = children->next) != NULL)
    if (position-- == 0)
      return children->node;
  return NULL;
}

struct node_list *newlist(struct node *n) {
  struct node_list *newlist = malloc(sizeof(struct node_list));
  if (newlist == NULL)
    return newlist;
  newlist->node = n;
  newlist->next = NULL;
  return newlist;
}

struct node_list *append(struct node_list *list, struct node *n) {
  if (n == NULL)
    return list;
  if (list == NULL)
    return newlist(n);

  struct node_list *current_node = list;
  while (current_node->next != NULL) {
    if (current_node->node == n)
      return list;
    current_node = current_node->next;
  }

  if (current_node->node == n)
    return list;

  current_node->next = newlist(n);
  return list;
};

void addchildren(struct node *parent, struct node_list *list) {
  if (parent != NULL && list != NULL) {
    struct node_list *curr = parent->children;
    while (curr->next != NULL) {
      curr = curr->next;
    }
    curr->next = list;
  }
}

void yyerror(const char *s) {
  /* Using your existing line and collumn variables */
  extern int line, collumn;
  extern char *string_buffer;
  extern int start_string_column;
  extern char *yytext;
  extern int has_errors;
  has_errors = 1;

  if (yytext[0] == '"' && strlen(yytext) == 1) {
    printf("Line %d, col %d: %s: \"%s\"\n", line, start_string_column, s,
           string_buffer);
  } else {
    printf("Line %d, col %d: %s: %s\n", line, (int)(collumn - strlen(yytext)),
           s, yytext);
  }
}

struct node *copy_node(struct node *n) {
  if (n == NULL)
    return NULL;

  struct node *copy = calloc(1, sizeof(struct node));
  if (copy == NULL)
    return NULL;

  copy->category = n->category;
  copy->type = n->type;
  copy->line = n->line;
  copy->column = n->column;
  copy->is_duplicate = n->is_duplicate;

  if (n->token != NULL) {
    copy->token = strdup(n->token);
  }

  if (n->parameter_types_str != NULL) {
    copy->parameter_types_str = strdup(n->parameter_types_str);
  }

  copy->children = copy_list(n->children);

  return copy;
}

struct node_list *copy_list(struct node_list *list) {
  if (list == NULL)
    return NULL;

  struct node_list *new_l = malloc(sizeof(struct node_list));
  if (new_l == NULL)
    return NULL;

  new_l->node = copy_node(list->node);
  new_l->next = copy_list(list->next);

  return new_l;
}

int count_list(struct node_list *list) {
  if (list == NULL) {
    return 0;
  }

  int count = 0;

  struct node_list *curr = list;
  while (curr != NULL) {
    count++;
    curr = curr->next;
  }

  return count;
}

void free_ast(struct node *n) {
  if (n == NULL)
    return;

  struct node_list *curr = n->children;
  while (curr != NULL) {
    struct node_list *temp = curr;
    curr = curr->next;

    free_ast(temp->node);

    free(temp);
  }

  // if (n->token != NULL) {
  //   free(n->token);
  // }

  free(n);
}

const char *get_node_string(category cat) { return node_names[cat]; }
