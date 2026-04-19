#include "semantics.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int semantic_errors = 0;

struct symbol_list *symbol_table;

void check_method(struct node *method_decl) {
  struct node *method_header = getchild(method_decl, 0);
  struct node *type_node = getchild(method_header, 0);
  struct node *method_id = getchild(method_header, 1);
  struct node *parameters = getchild(method_header, 2);

  enum type return_type = category_type(type_node->category);

  if (search_symbol(symbol_table, method_id->token) == NULL) {
    insert_symbol(symbol_table, method_header->token, return_type, method_decl);
  } else {
    printf("Line %d, Column %d: Symbol %s already defined\n", method_id->line,
           method_id->column, method_id->token);
    semantic_errors++;
  }
  struct symbol_list *local_table = malloc(sizeof(struct symbol_list));
  local_table->next = NULL;
  local_table->identifier = NULL;
  insert_symbol(local_table, "return", return_type, NULL);

  check_parameters(parameters, local_table);
  // TODO check expression
  // check_expression(getchild(method_decl, 2), local_table);

  struct node *body = getchild(method_decl, 1);
  // TODO check body
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

enum type category_type(category c) {
  switch (c) {
  case Int:
  case Natural:
    return integer_type;

  case Double:
  case Decimal:
    return double_type;

  case Bool:
  case Boollit:
    return boolean_type;

  case StringArray:
    return string_array_type;

  case Void:
    return void_type;

  default:
    return undef_type;
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

void check_field_decl(struct node *field_decl,
                      struct symbol_list *local_table) {}

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
    } else if (child->node->category == FieldDecl) {
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

void print_tables(struct node *program) {
  printf("===== Class %s Symbol Table =====\n", symbol_table->identifier);
  struct symbol_list *s = symbol_table->next;
  while (s != NULL) {
    printf("%s\t%s\t%s\n", s->identifier, "(...)", type_to_string(s->type));
    s = s->next;
  }
  printf("\n");

  struct node_list *child = program->children->next;
  while (child != NULL) {
    if (child->node->category == MethodDecl) {
      struct node *header = getchild(child->node, 0);
      char *m_name = getchild(header, 1)->token;
      printf("===== Method %s(...) Symbol Table =====\n", m_name);

      struct symbol_list *local = child->node->local_symbols;
      struct symbol_list *ls = local->next;
      while (ls != NULL) {
        printf("%s\t\t%s%s\n", ls->identifier, type_to_string(ls->type),
               ls->is_parameter ? "\tparam" : "");
        ls = ls->next;
      }
      printf("\n");
    }
    child = child->next;
  }
}
