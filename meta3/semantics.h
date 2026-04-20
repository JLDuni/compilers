#ifndef SEMANTICS_H
#define SEMANTICS_H
#include "ast.h"
#include <stdbool.h>

int check_program(struct node *program);

struct symbol_list {
  char *identifier;
  enum type type;
  struct node *node;
  struct symbol_list *next;
  bool is_parameter;
};

struct symbol_list *insert_symbol(struct symbol_list *symbol_table,
                                  char *identifier, enum type type,
                                  struct node *node);
struct symbol_list *search_symbol(struct symbol_list *symbol_table,
                                  char *identifier);
void check_parameters(struct node *child, struct symbol_list *scope_table);
void check_expression(struct node *expr, struct symbol_list *scope_table);
enum type category_type(category c);
void check_var_decl(struct node *var_decl, struct symbol_list *local_table);

void check_method_body(struct node *method_body,
                       struct symbol_list *local_table);
char *type_to_string(enum type type);

void check_statement(struct node *statement_body,
                     struct symbol_list *local_table);
void show_symbol_table();

bool is_reserved_underscore(struct node *id_node);

#endif
