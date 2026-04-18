#include "semantics.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int semantic_errors = 0;

struct symbol_list *symbol_table;

void check_function(struct node *function) {
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

void check_expression(struct node *expression,
                      struct symbol_list *scope_table) {
  if (expression == NULL)
    return;

  if (expression->category == Natural) {
    expression->type = integer_type;
  } else if (expression->category == Double) {
    expression->type = double_type;
  } else if (expression->category == Identifier) {
    struct symbol_list *list = search_symbol(scope_table, expression->token);
    if (list != NULL) {
      expression->type = list->type;
    } else {
      printf("Line %d, Column %d: Symbol %s is not defined\n", expression->line,
             expression->column, expression->token);
      expression->type = no_type;
      semantic_errors++;
    }
  }

  struct node_list *children = expression->children;
  while (children != NULL) {
    check_expression(children->node, scope_table);
    children = children->next;
  }

  if (expression->category == Add || expression->category == Sub ||
      expression->category == Mul || expression->category == Div) {

    enum type t1 = getchild(expression, 0)->type;
    enum type t2 = getchild(expression, 1)->type;

    if (t1 == no_type) {
      expression->type = t2;
    } else if (t2 == no_type) {
      expression->type = t1;
    } else if (t1 == t2) {
      expression->type = t1;
    } else {
      printf("Line %d, col %d: Incompatible types in operation\n",
             expression->line, expression->column);
      expression->type = no_type;
      semantic_errors++;
    }
  }

  if (expression->category == Call) {
    struct node *id = getchild(expression, 0);
    struct symbol_list *sym = search_symbol(symbol_table, id->token);
    if (sym != NULL)
      expression->type = sym->type;
    else {
      printf("Line %d, col %d: Symbol %s not defined\n", id->line, id->column,
             id->token);
      expression->type = no_type;
      semantic_errors++;
    }
  }
}

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

// semantic analysis begins here, with the AST root node
int check_program(struct node *program) {
  symbol_table = (struct symbol_list *)malloc(sizeof(struct symbol_list));
  symbol_table->next = NULL;
  struct node_list *child = program->children;
  while ((child = child->next) != NULL)
    check_function(child->node);
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
