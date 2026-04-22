#include "semantics.h"
#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int semantic_errors = 0;

struct symbol_list *symbol_table;

void check_method(struct node *method_decl) {
  struct node *method_header = getchild(method_decl, 0);
  struct node *method_body = getchild(method_decl, 1);

  struct node *type_node = getchild(method_header, 0);
  struct node *method_id = getchild(method_header, 1);
  struct node *parameters = getchild(method_header, 2);

  enum type return_type = category_type(type_node->category);

  if (search_symbol(symbol_table, method_id->token) == NULL) {
    insert_symbol(symbol_table, method_id->token, return_type, method_decl);
  } else {
    printf("Line %d, col %d: Symbol %s already defined\n", method_id->line,
           method_id->column, method_id->token);
    semantic_errors++;
  }
  struct symbol_list *local_table = malloc(sizeof(struct symbol_list));
  local_table->next = NULL;
  local_table->identifier = NULL;
  insert_symbol(local_table, "return", return_type, NULL);

  method_decl->local_symbols = local_table;

  check_parameters(parameters, local_table);

  struct node *body = getchild(method_decl, 1);
  check_method_body(body, local_table);
}

void check_method_body(struct node *method_body,
                       struct symbol_list *local_table) {
  if (method_body == NULL || local_table == NULL) {
    return;
  }
  struct node_list *curr = method_body->children;
  while (curr != NULL) {
    struct node *child = curr->node;
    if (child != NULL) {
      if (child->category == VarDecl) {
        check_var_decl(child, local_table);
      } else {
        check_statement(child, local_table);
      }
    }
    curr = curr->next;
  }
}

void check_statement(struct node *statement_body,
                     struct symbol_list *local_table) {
  if (statement_body == NULL) {
    return;
  }

  struct node *expression = getchild(statement_body, 0);

  switch (statement_body->category) {
  case If:
    check_expression(expression, local_table);
    if (expression->type != boolean_type) {
      printf("Line %d, col %d: Incompatible type %s in if statement\n",
             expression->line, expression->column,
             type_to_string(category_type(expression->category)));
      semantic_errors++;
    }
    check_statement(getchild(statement_body, 1), local_table);
    check_statement(getchild(statement_body, 2), local_table);
    break;
  case While:
    check_expression(expression, local_table);
    if (expression->type != boolean_type) {
      printf("Line %d, col %d: Incompatible type %s in %s statement\n",
             expression->line, expression->column,
             type_to_string(expression->type), expression->token);
      semantic_errors++;
    }
    check_statement(getchild(statement_body, 1), local_table);
    break;
  case Return: {
    struct symbol_list *return_sym = search_symbol(local_table, "return");
    enum type expected_type = return_sym != NULL ? return_sym->type : void_type;

    enum type returned_type = void_type;

    struct node *expr = getchild(statement_body, 0);

    if (expr != NULL) {
      check_expression(expr, local_table);
      returned_type = expr->type;
    }

    bool is_compatible = false;
    if (expected_type == void_type && expr != NULL) {
      is_compatible = false;
    } else if (expected_type == returned_type) {
      is_compatible = true;
    } else if (expected_type == double_type && returned_type == integer_type) {
      is_compatible = true;
    }

    if (!is_compatible && returned_type != undef_type) {
      int err_line = (expr != NULL) ? expr->line : statement_body->line;
      int err_col = (expr != NULL) ? expr->column : statement_body->column;

      printf("Line %d, col %d: Incompatible type %s in return statement\n",
             err_line, err_col, type_to_string(returned_type));
      semantic_errors++;
    }
    break;
  }
  case Assign:
    check_expression(statement_body, local_table);
    break;
  case Print:
    struct node *print = getchild(statement_body, 0);
    if (print->category != StrLit) {
      check_expression(print, local_table);
    }
    break;

  case Block:
    if (statement_body->children != NULL) {
      struct node_list *statements = statement_body->children;
      while (statements != NULL) {
        struct node *statement_node = statements->node;
        check_expression(statement_node, local_table);
        statements = statements->next;
      }
    }
    break;

  default:
    if (statement_body->category == Call ||
        statement_body->category == ParseArgs) {
      check_expression(statement_body, local_table);
    }
    break;
  }
}

void check_expression(struct node *expr, struct symbol_list *local_scope) {
  if (expr == NULL)
    return;
  struct node_list *child_ptr =
      (expr->children != NULL) ? expr->children->next : NULL;

  if (expr->category == Call && child_ptr != NULL) {
    child_ptr = child_ptr->next;
  }
  while (child_ptr != NULL) {
    check_expression(child_ptr->node, local_scope);
    child_ptr = child_ptr->next;
  }

  switch (expr->category) {
  case Natural:
    char *clean_token = clean_token_underscores(expr->token);
    long long value = atoll(clean_token);
    if (value > 2147483647LL) {
      printf("Line %d, col %d: Number %s out of bounds\n", expr->line,
             expr->column, expr->token);
      semantic_errors++;
    }
    expr->type = integer_type;
    break;
  case Decimal:
    expr->type = double_type;
    break;
  case BoolLit:
    expr->type = boolean_type;
    break;
  case Identifier: {
    struct symbol_list *sym = search_symbol(local_scope, expr->token);
    if (sym != NULL) {
      expr->type = sym->type;
    } else {
      printf("Line %d, col %d: Cannot find symbol %s\n", expr->line,
             expr->column, expr->token);
      expr->type = undef_type;
      semantic_errors++;
    }
    break;
  }

  case Minus:
  case Plus: {
    struct node *child = getchild(expr, 0);
    enum type t = child->type;

    if (t == integer_type || t == double_type) {
      expr->type = t;
    } else if (t != undef_type) {
      printf("Line %d, col %d: Operator %s cannot be applied to type %s\n",
             expr->line, expr->column, get_symbol_category(expr->category),
             type_to_string(t));
      expr->type = undef_type;
      semantic_errors++;
    } else {
      expr->type = undef_type;
    }
    break;
  }

  case Not: {
    struct node *child = getchild(expr, 0);
    enum type t = child->type;

    if (t == boolean_type) {
      expr->type = boolean_type;
    } else if (t != undef_type) {
      printf("Line %d, col %d: Operator ! cannot be applied to type %s\n",
             expr->line, expr->column, type_to_string(t));
      expr->type = undef_type;
      semantic_errors++;
    } else {
      expr->type = undef_type;
    }
    break;
  }

  case Add:
  case Sub:
  case Mul:
  case Div:
  case Mod: {
    enum type t1 = getchild(expr, 0)->type;
    enum type t2 = getchild(expr, 1)->type;

    if ((t1 == integer_type || t1 == double_type) &&
        (t2 == integer_type || t2 == double_type)) {
      expr->type =
          (t1 == double_type || t2 == double_type) ? double_type : integer_type;
    } else {
      printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n",
             expr->line, expr->column, get_symbol_category(expr->category),
             type_to_string(t1), type_to_string(t2));
      expr->type = undef_type;
      semantic_errors++;
    }
    break;
  }

  case Eq:
  case Ne:
  case Lt:
  case Gt:
  case Le:
  case Ge: {
    enum type t1 = getchild(expr, 0)->type;
    enum type t2 = getchild(expr, 1)->type;

    expr->type = boolean_type;

    if (!((t1 == t2) || ((t1 == integer_type || t1 == double_type) &&
                         (t2 == integer_type || t2 == double_type)))) {
      printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n",
             expr->line, expr->column, get_symbol_category(expr->category),
             type_to_string(t1), type_to_string(t2));
    }
    break;
  }

  case And:
  case Or:
  case Xor: {
    expr->type = boolean_type;
    break;
  }

  case ParseArgs: {
    struct node *id_node = getchild(expr, 0);
    struct node *index_expr = getchild(expr, 1);

    if (id_node == NULL || index_expr == NULL) {
      expr->type = integer_type;
      break;
    }

    enum type t1 = id_node->type;
    enum type t2 = index_expr->type;

    if (!((t1 == string_array_type || t1 == undef_type) &&
          (t2 == integer_type || t2 == undef_type))) {

      printf("Line %d, col %d: Operator Integer.parseInt cannot be applied to "
             "types %s, %s\n",
             expr->line, expr->column, type_to_string(t1), type_to_string(t2));
      semantic_errors++;
    }
    expr->type = integer_type;
    break;
  }

  case Length: {
    struct node *child = getchild(expr, 0);

    if (child == NULL) {
      expr->type = integer_type;
      break;
    }

    enum type t = child->type;

    if (t != string_array_type && t != undef_type) {
      printf("Line %d, col %d: Operator .length cannot be applied to type %s\n",
             expr->line, expr->column, type_to_string(t));
      semantic_errors++;
    }
    expr->type = integer_type;
    break;
  }

  case Call: {
    struct node *id_node = getchild(expr, 0);
    int compatible_count_methods = 0;
    struct symbol_list *sym = find_correspondent_method(
        id_node->token, expr, &compatible_count_methods);

    if (compatible_count_methods > 1) {
      printf("Line %d, col %d: Reference to method %s is ambiguous",
             id_node->line, id_node->column, id_node->token);
      expr->type = undef_type;
      id_node->type = undef_type;
      semantic_errors++;
    } else if (sym == NULL) {
      printf("Line %d, col %d: Cannot find symbol %s(", id_node->line,
             id_node->column, id_node->token);

      struct node_list *arg_ptr = expr->children->next->next;

      while (arg_ptr != NULL) {
        printf("%s", type_to_string(arg_ptr->node->type));
        if (arg_ptr->next != NULL) {
          printf(",");
        }
        arg_ptr = arg_ptr->next;
      }
      printf(")\n");

      expr->type = undef_type;
      id_node->type = undef_type;
      semantic_errors++;
    } else {
      expr->type = sym->type;
      id_node->type = sym->type;
    }
    break;
  }

  case Assign: {
    enum type t_id = getchild(expr, 0)->type;
    enum type t_val = getchild(expr, 1)->type;
    expr->type = t_id;

    if (t_id != t_val && !(t_id == double_type && t_val == integer_type)) {
      printf("Line %d, col %d: Operator = cannot be applied to types %s, %s\n",
             expr->line, expr->column, type_to_string(t_id),
             type_to_string(t_val));
      semantic_errors++;
    }
    break;
  }

  default:
    expr->type = undef_type;
    break;
  }
}

char *get_symbol_category(category cat) {
  switch (cat) {
  case Assign:
    return "=";
  case Or:
    return "||";
  case And:
    return "&&";
  case Eq:
    return "==";
  case Ne:
    return "!=";
  case Lt:
    return "<";
  case Gt:
    return ">";
  case Le:
    return "<=";
  case Ge:
    return ">=";
  case Add:
    return "+";
  case Sub:
    return "-";
  case Mul:
    return "*";
  case Div:
    return "/";
  case Mod:
    return "%";
  case Lshift:
    return "<<";
  case Rshift:
    return ">>";
  case Xor:
    return "^";
  case Not:
    return "!";
  case Minus:
    return "-"; // Unário
  case Plus:
    return "+"; // Unário
  default:
    return ""; // Retorno de segurança para não dar (null)
  }
}

struct symbol_list *find_correspondent_method(char *call_identifier,
                                              struct node *call_node,
                                              int *compatible_count) {
  struct node_list *call_id_list = call_node->children;
  int args_count = count_list(call_id_list) - 2;
  if (call_identifier == NULL || call_node == NULL) {
    return NULL;
  }
  struct symbol_list *best_match = NULL;

  struct symbol_list *curr_symbol = symbol_table;
  while (curr_symbol != NULL) {
    if (strcmp(curr_symbol->identifier, call_identifier) == 0) {
      struct node *curr_node = curr_symbol->node;
      struct node *params = getchild(getchild(curr_node, 0), 2);
      struct node_list *parameters_list = params->children;
      int method_params_count = count_list(parameters_list) - 1;
      if (args_count == method_params_count) {
        bool exact_match = true;
        bool compatible_match = true;
        for (int i = 0; i < args_count; i++) {
          struct node *id = getchild(call_node, i + 1);
          category param_category = getchild(getchild(params, i), 0)->category;

          enum type param_type = category_type(param_category);
          enum type call_id_type = id->type;

          if (param_type != call_id_type) {
            exact_match = false;
            if (!(param_type == double_type && call_id_type == integer_type)) {
              compatible_match = false;
              break;
            }
          }
        }
        if (exact_match) {
          return curr_symbol;
        }

        if (compatible_match) {
          best_match = curr_symbol;
          (*compatible_count)++;
        }
      }
    }
    curr_symbol = curr_symbol->next;
  }
  return best_match;
}

void check_var_decl(struct node *var_decl, struct symbol_list *local_table) {
  struct node *type = getchild(var_decl, 0);
  enum type nodes_type = category_type(type->category);

  int i = 1;
  struct node *id = getchild(var_decl, i);

  while ((id = getchild(var_decl, i)) != NULL) {
    if (is_reserved_underscore(id)) {
      i++;
      continue;
    }

    if (search_symbol(local_table, id->token) != NULL) {
      printf("Line %d, col %d: Symbol %s already defined\n", id->line,
             id->column, id->token);
      semantic_errors++;
    } else {
      insert_symbol(local_table, id->token, nodes_type, id);
    }
    i++;
  }
}

char *clean_token_underscores(char *token) {
  char *clean_token = malloc(strlen(token) + 1);
  int j = 0;
  for (int i = 0; token[i] != '\0'; i++) {
    if (token[i] != '_') {
      clean_token[j++] = token[i];
    }
  }
  clean_token[j] = '\0';
  return clean_token;
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

      if (!is_reserved_underscore(id)) {
        char *identifier = id->token;
        if (search_symbol(scope_table, identifier) != NULL) {
          printf("Line %d, col %d: Symbol %s already defined\n", id->line,
                 id->column, id->token);
          semantic_errors++;
          return;
        } else {
          enum type p_type = category_type(type->category);
          struct symbol_list *new_symbol =
              insert_symbol(scope_table, id->token, p_type, type);
          new_symbol->is_parameter = 1;
        }
      }
    }
    parameter = parameter->next;
  }
}

void check_field_decl(struct node *field_decl,
                      struct symbol_list *global_table) {
  if (field_decl == NULL)
    return;

  struct node *type_node = getchild(field_decl, 0);
  struct node *id_node = getchild(field_decl, 1);

  enum type field_type = category_type(type_node->category);

  struct symbol_list *existing_symbol =
      search_symbol(global_table, id_node->token);

  if (existing_symbol != NULL) {
    printf("Line %d, col %d: Symbol %s already defined\n", id_node->line,
           id_node->column, id_node->token);
    semantic_errors++;
  } else {
    insert_symbol(global_table, id_node->token, field_type, NULL);
  }
}

// semantic analysis begins here, with the AST root node
int check_program(struct node *program) {
  struct node *class_id = getchild(program, 0);

  symbol_table = (struct symbol_list *)malloc(sizeof(struct symbol_list));
  symbol_table->next = NULL;
  if (class_id != NULL && class_id->token != NULL) {
    symbol_table->identifier = strdup(class_id->token);
  }
  symbol_table->identifier = strdup(class_id->token);
  struct node_list *child = program->children;
  while ((child = child->next) != NULL) {
    if (child->node->category == MethodDecl) {
      check_method(child->node);
    } else if (child->node->category == FieldDecl) {
      check_field_decl(child->node, symbol_table);
    }
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
  case undef_type:
    return "undef";
  default:
    return "";
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
  case BoolLit:
    return boolean_type;

  case StringArray:
    return string_array_type;

  case Void:
    return void_type;

  default:
    return undef_type;
  }
}

bool is_reserved_underscore(struct node *id_node) {
  if (id_node != NULL && id_node->token != NULL) {
    if (strcmp(id_node->token, "_") == 0) {
      printf("Line %d, col %d: Symbol _ is reserved\n", id_node->line,
             id_node->column);
      semantic_errors++;
      return true;
    }
  }
  return false;
}

void print_tables(struct node *program) {
  printf("===== Class %s Symbol Table =====\n", symbol_table->identifier);
  struct symbol_list *s = symbol_table->next;
  while (s != NULL) {
    if (s->node != NULL && s->node->category == MethodDecl) {
      printf("%s\t", s->identifier);
      print_method_params(s->node);
      printf("\t%s\n", type_to_string(s->type));
    } else {
      printf("%s\t\t%s\n", s->identifier, type_to_string(s->type));
    }
    s = s->next;
  }
  printf("\n");

  struct node_list *child = program->children->next;
  while (child != NULL) {
    if (child->node->category == MethodDecl) {
      struct node *header = getchild(child->node, 0);
      char *m_name = getchild(header, 1)->token;

      printf("===== Method %s", m_name);
      print_method_params(child->node);
      printf(" Symbol Table =====\n");

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

void print_method_params(struct node *method_decl) {
  printf("(");
  struct node *method_header = getchild(method_decl, 0);
  struct node *params = getchild(method_header, 2);

  if (params != NULL && params->children != NULL &&
      params->children->next != NULL) {

    struct node_list *curr_param = params->children->next;

    while (curr_param != NULL) {
      struct node *param_decl = curr_param->node;

      if (param_decl != NULL) {
        struct node *type_node = getchild(param_decl, 0);
        if (type_node != NULL) {
          printf("%s", type_to_string(category_type(type_node->category)));
        }
      }

      if (curr_param->next != NULL) {
        printf(",");
      }
      curr_param = curr_param->next;
    }
  }
  printf(")");
}
