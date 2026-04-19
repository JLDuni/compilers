#include "semantics.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int semantic_errors = 0;

struct symbol_list *symbol_table;

void check_method(struct node *function) {
  struct node *id = getchild(function, 0);
  if (search_symbol(symbol_table, id->token) == NULL) {
    insert_symbol(symbol_table, id->token, no_type, function);
  } else {
    printf("Identifier %s already declared\n", id->token);
    semantic_errors++;
  }
  struct symbol_list *local_table = malloc(sizeof(struct symbol_list));
  local_table->next = NULL;
  local_table->identifier = NULL;
  check_parameters(getchild(function, 1), local_table);
  check_expression(getchild(function, 2), local_table);
}

char *type_to_string(enum type type) {
  switch (type) {
  case integer_type:
    return "int";
  case double_type:
    return "double";
  case boolean_type:
    return "boolean";
  case string_array_type:
    return "String[]";
  case void_type:
    return "void";
  default:
    return "undef";
  }
}

// void check_expression(struct node *expression,
//                       struct symbol_list *scope_table) {
// }

void check_parameters(struct node *parameters,
                      struct symbol_list *scope_table) {
  if (parameters == NULL) {
    return;
  }
  struct node_list *parameter = parameters->children;
  while (parameter != NULL) {
    struct node *current_parameter = parameter->node;
    if (current_parameter != NULL) {
      struct node *id = getchild(current_parameter, 1);
      struct node *type = getchild(current_parameter, 0);
      char *identifier = id->token;
      if (search_symbol(scope_table, identifier) != NULL) {
        printf("Identifier already in symbol table in line %d, column %d\n",
               id->line, id->column);
        semantic_errors++;
        return;
      } else {
        enum type p_type = category_type(type->category);
        insert_symbol(scope_table, id->token, p_type, type);
      }
    }
    parameter = parameter->next;
  }
}

void check_field_decl(struct node *field_decl, struct symbol_list *global_table) {

}

// semantic analysis begins here, with the AST root node
int check_program(struct node *program) {
  struct node *class_id = getchild(program, 0);

  symbol_table = (struct symbol_list *)malloc(sizeof(struct symbol_list));
  symbol_table->next = NULL;
  symbol_table->identifier = strdup(class_id->token);
  struct node_list *child = program->children;
  while ((child = child->next) != NULL) {
    if (child->node->category == MethodDecl) {
      check_method(child->node);
    } else if (child->node->category == ParamDecl) {
      check_field_decl(child->node, symbol_table);
    }
    child = child->next;
  }

  return semantic_errors;
}

// insert a new symbol in the list, unless it is already there
struct symbol_list *insert_symbol(struct symbol_list *table, char *identifier,
                                  enum type type, struct node *node) {
  if (search_symbol(table, identifier) != NULL)
    return NULL; /* return NULL if symbol is already inserted */
  struct symbol_list *new =
      (struct symbol_list *)malloc(sizeof(struct symbol_list));
  new->identifier = strdup(identifier);
  new->type = type;
  new->node = node;
  new->next = NULL;
  struct symbol_list *symbol = table;
  while (symbol->next != NULL)
    symbol = symbol->next;
  symbol->next = new; /* insert new symbol at the tail of the list */
  return new;
}

// look up a symbol by its identifier
struct symbol_list *search_symbol(struct symbol_list *table, char *identifier) {
  if (identifier == NULL) {
    return NULL;
  }
  struct symbol_list *symbol;
  for (symbol = table->next; symbol != NULL; symbol = symbol->next)
    if (strcmp(symbol->identifier, identifier) == 0)
      return symbol;
  return NULL;
}

void show_symbol_table() {
  struct symbol_list *symbol;
  for (symbol = symbol_table->next; symbol != NULL; symbol = symbol->next)
    printf("Symbol %s : %s\n", symbol->identifier, type_name(symbol->type));
}
