#include "semantics.h"
#include "ast.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int semantic_errors = 0;

struct symbol_list *symbol_table;

void verify_parameters(struct node *parameters) {
  if (parameters == NULL || parameters->children == NULL)
    return;

  struct node_list *p_list = parameters->children->next;

  struct symbol_list *temp = malloc(sizeof(struct symbol_list));
  if (temp == NULL)
    return;

  temp->next = NULL;
  temp->identifier = NULL;
  temp->node = NULL;

  while (p_list != NULL) {
    if (p_list->node != NULL) {
      struct node *id = getchild(p_list->node, 1);

      if (id != NULL && id->token != NULL) {
        if (strcmp(id->token, "_") == 0) {
          printf("Line %d, col %d: Symbol _ is reserved\n", id->line,
                 id->column);
          semantic_errors++;
        } else if (search_symbol(temp, id->token) != NULL) {
          printf("Line %d, col %d: Symbol %s already defined\n", id->line,
                 id->column, id->token);
          semantic_errors++;
        } else {
          insert_symbol(temp, id->token, integer_type, NULL);
        }
      }
    }
    p_list = p_list->next;
  }

  struct symbol_list *curr = temp;
  while (curr != NULL) {
    struct symbol_list *next_node = curr->next;
    free(curr);
    curr = next_node;
  }
}

int compare_parameters(struct node *params1, struct node *params2) {
  struct node_list *p1 = (params1 != NULL && params1->children != NULL)
                             ? params1->children->next
                             : NULL;
  struct node_list *p2 = (params2 != NULL && params2->children != NULL)
                             ? params2->children->next
                             : NULL;

  while (p1 != NULL && p2 != NULL) {
    struct node *t1 = getchild(p1->node, 0);
    struct node *t2 = getchild(p2->node, 0);

    if (t1 == NULL || t2 == NULL) {
      return 0;
    }

    if (category_type(t1->category) != category_type(t2->category)) {
      return 0;
    }
    p1 = p1->next;
    p2 = p2->next;
  }

  return (p1 == NULL && p2 == NULL);
}

void add_method_to_table(struct node *method_decl) {
  if (method_decl == NULL || method_decl->children == NULL)
    return;

  method_decl->is_duplicate = 0;

  struct node *method_header = getchild(method_decl, 0);
  if (method_header == NULL)
    return;

  struct node *method_id = getchild(method_header, 1);
  struct node *parameters = getchild(method_header, 2);
  struct node *type_node = getchild(method_header, 0);

  if (method_id == NULL || method_id->token == NULL || type_node == NULL) {
    return;
  }
  if (strcmp(method_id->token, "_") == 0) {
    is_reserved_underscore(method_id);
    method_decl->is_duplicate = 1;
    method_decl->local_symbols = NULL;
  }

  enum type return_type = category_type(type_node->category);
  int is_duplicate = 0;

  verify_parameters(parameters);

  struct symbol_list *curr = symbol_table;

  while (curr != NULL) {
    if (curr->identifier != NULL &&
        strcmp(curr->identifier, method_id->token) == 0) {
      if (curr->node != NULL && curr->node->category == MethodDecl) {
        struct node *existing_header = getchild(curr->node, 0);
        if (existing_header != NULL) {
          struct node *existing_params = getchild(existing_header, 2);

          if (compare_parameters(parameters, existing_params)) {
            is_duplicate = 1;
            break;
          }
        }
      }
    }
    curr = curr->next;
  }

  if (!is_duplicate) {
    insert_symbol(symbol_table, method_id->token, return_type, method_decl);
    method_decl->is_duplicate = 0;
  } else {
    printf("Line %d, col %d: Symbol %s", method_id->line, method_id->column,
           method_id->token);
    print_method_params(method_decl);
    printf(" already defined\n");

    semantic_errors++;
    method_decl->is_duplicate = 1;
    method_decl->local_symbols = NULL;
  }
}

void check_method_semantics(struct node *method_decl) {
  if (method_decl == NULL)
    return;
  if (method_decl->is_duplicate)
    return;

  struct node *method_header = getchild(method_decl, 0);
  if (method_header == NULL)
    return;

  struct node *type_node = getchild(method_header, 0);
  if (type_node == NULL)
    return;

  struct node *parameters = getchild(method_header, 2);
  struct node *body = getchild(method_decl, 1);

  struct symbol_list *local_table = malloc(sizeof(struct symbol_list));
  if (local_table == NULL)
    return;

  local_table->next = NULL;
  local_table->identifier = NULL;
  local_table->node = NULL;
  local_table->type = undef_type;

  insert_symbol(local_table, "return", category_type(type_node->category),
                NULL);

  method_decl->local_symbols = local_table;

  check_parameters(parameters, local_table);
  check_method_body(body, local_table);
}

void check_method(struct node *method_decl) {
  if (method_decl == NULL)
    return;

  struct node *method_header = getchild(method_decl, 0);

  struct node *type_node = getchild(method_header, 0);
  struct node *method_id = getchild(method_header, 1);
  struct node *parameters = getchild(method_header, 2);

  if (type_node == NULL || method_id == NULL || method_id->token == NULL)
    return;

  enum type return_type = category_type(type_node->category);
  int is_duplicate = 0;
  struct symbol_list *curr = symbol_table;

  while (curr != NULL) {
    if (curr->identifier != NULL &&
        strcmp(curr->identifier, method_id->token) == 0) {
      if (curr->node != NULL && curr->node->category == MethodDecl) {

        struct node *existing_header = getchild(curr->node, 0);
        if (existing_header != NULL) {
          struct node *existing_params = getchild(existing_header, 2);
          if (compare_parameters(parameters, existing_params)) {
            is_duplicate = 1;
            break;
          }
        }
      }
    }
    curr = curr->next;
  }

  if (!is_duplicate) {
    insert_symbol(symbol_table, method_id->token, return_type, method_decl);

    struct symbol_list *local_table = malloc(sizeof(struct symbol_list));
    if (local_table == NULL)
      return;

    local_table->next = NULL;
    local_table->identifier = NULL;
    local_table->node = NULL;

    insert_symbol(local_table, "return", return_type, NULL);
    method_decl->local_symbols = local_table;

    check_parameters(parameters, local_table);

    struct node *body = getchild(method_decl, 1);
    check_method_body(body, local_table);

  } else {
    printf("Line %d, col %d: Symbol %s", method_id->line, method_id->column,
           method_id->token);
    print_method_params(method_decl);
    printf(" already defined\n");
    semantic_errors++;
  }
}

void check_method_body(struct node *method_body,
                       struct symbol_list *local_table) {
  if (method_body == NULL || local_table == NULL) {
    return;
  }

  if (method_body->children == NULL) {
    return;
  }

  struct node_list *curr = method_body->children->next;

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
  if (statement_body == NULL)
    return;

  struct node *expression = getchild(statement_body, 0);

  switch (statement_body->category) {
  case If: {
    if (expression != NULL) {
      check_expression(expression, local_table);
      if (expression->type != boolean_type) {
        printf("Line %d, col %d: Incompatible type %s in if statement\n",
               expression->line, expression->column,
               type_to_string(expression->type));
        semantic_errors++;
      }
    }
    check_statement(getchild(statement_body, 1), local_table);
    check_statement(getchild(statement_body, 2), local_table);
    break;
  }

  case While:
    if (expression != NULL) {
      check_expression(expression, local_table);
      if (expression->type != boolean_type) {
        printf("Line %d, col %d: Incompatible type %s in while statement\n",
               expression->line, expression->column,
               type_to_string(expression->type));
        semantic_errors++;
      }
    }
    check_statement(getchild(statement_body, 1), local_table);
    break;

  case Return: {
    struct symbol_list *return_sym = search_symbol(local_table, "return");
    enum type expected_type =
        (return_sym != NULL) ? return_sym->type : void_type;
    enum type returned_type = void_type;

    if (expression != NULL) {
      check_expression(expression, local_table);
      returned_type = expression->type;
    }

    bool is_compatible = false;
    if (expected_type == void_type && expression != NULL)
      is_compatible = false;
    else if (expected_type == returned_type)
      is_compatible = true;
    else if (expected_type == double_type && returned_type == integer_type)
      is_compatible = true;

    if (!is_compatible) {
      int err_line =
          (expression != NULL) ? expression->line : statement_body->line;
      int err_col =
          (expression != NULL) ? expression->column : statement_body->column;
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
    if (expression != NULL) {
      check_expression(expression, local_table);
      enum type t = expression->type;
      if (t != integer_type && t != double_type && t != boolean_type &&
          t != string_type) {
        printf("Line %d, col %d: Incompatible type %s in System.out.print "
               "statement\n",
               expression->line, expression->column, type_to_string(t));
        semantic_errors++;
      }
    }
    break;

  case Block: {
    if (statement_body->children != NULL) {
      struct node_list *curr = statement_body->children;
      while (curr != NULL) {
        if (curr->node != NULL) {
          if (curr->node->category == VarDecl)
            check_var_decl(curr->node, local_table);
          else
            check_statement(curr->node, local_table);
        }
        curr = curr->next;
      }
    }
    break;
  }

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
  case Natural: {
    if (expr->token == NULL)
      break;

    char const *clean_token = clean_token_underscores(expr->token);
    int len = strlen(clean_token);
    int is_out_of_bounds = 0;

    if (len > 10) {
      is_out_of_bounds = 1;
    } else if (len == 10 && strcmp(clean_token, "2147483648") >= 0) {
      is_out_of_bounds = 1;
    }

    if (is_out_of_bounds) {
      printf("Line %d, col %d: Number %s out of bounds\n", expr->line,
             expr->column, expr->token);
      semantic_errors++;
    }
    expr->type = integer_type;
    free((void *)clean_token);
    break;
  }

  case Decimal: {
    char *clean = clean_token_underscores(expr->token);
    double val = strtod(clean, NULL);

    if (val == HUGE_VAL || val == -HUGE_VAL) {
      printf("Line %d, col %d: Number %s out of bounds\n", expr->line,
             expr->column, expr->token);
      semantic_errors++;
    } else if (val == 0.0) {
      int has_value = 0;
      for (int i = 0; clean[i]; i++)
        if (clean[i] >= '1' && clean[i] <= '9') {
          has_value = 1;
          break;
        }

      if (has_value) {
        printf("Line %d, col %d: Number %s out of bounds\n", expr->line,
               expr->column, expr->token);
        semantic_errors++;
      }
    }

    free(clean);
    expr->type = double_type;
    break;
  }
  case BoolLit:
    expr->type = boolean_type;
    break;

  case StrLit:
    expr->type = string_type;
    break;

  case Identifier: {
    struct symbol_list *sym = NULL;

    sym = search_symbol(local_scope, expr->token);

    if (sym == NULL && symbol_table != NULL) {
      struct symbol_list *curr = symbol_table->next;
      while (curr != NULL) {
        if (curr->identifier && strcmp(curr->identifier, expr->token) == 0) {
          if (curr->node && curr->node->category == FieldDecl) {
            sym = curr;
            break;
          }
        }
        curr = curr->next;
      }
    }

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
    } else {
      printf("Line %d, col %d: Operator %s cannot be applied to type %s\n",
             expr->line, expr->column, get_symbol_category(expr->category),
             type_to_string(t));
      expr->type = undef_type;
      semantic_errors++;
    }
    break;
  }

  case Not: {
    struct node *child = getchild(expr, 0);
    enum type t = child->type;

    expr->type = boolean_type;

    if (t != boolean_type) {
      printf("Line %d, col %d: Operator ! cannot be applied to type %s\n",
             expr->line, expr->column, type_to_string(t));
      semantic_errors++;
    }
    break;
  }

  case Add:
  case Sub:
  case Mul:
  case Div:
  case Mod: {
    struct node *c1 = getchild(expr, 0);
    struct node *c2 = getchild(expr, 1);
    enum type t1 = c1->type;
    enum type t2 = c2->type;

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

  case Lt:
  case Gt:
  case Le:
  case Ge: {
    struct node *c1 = getchild(expr, 0);
    struct node *c2 = getchild(expr, 1);

    enum type t1 = c1->type;
    enum type t2 = c2->type;

    expr->type = boolean_type;

    int is_numeric1 = (t1 == integer_type || t1 == double_type);
    int is_numeric2 = (t2 == integer_type || t2 == double_type);

    if (!(is_numeric1 && is_numeric2)) {
      printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n",
             expr->line, expr->column, get_symbol_category(expr->category),
             type_to_string(t1), type_to_string(t2));
      semantic_errors++;
    }
    break;
  }

  case Eq:
  case Ne: {
    enum type t1 = getchild(expr, 0)->type;
    enum type t2 = getchild(expr, 1)->type;

    expr->type = boolean_type;

    int valid = 0;
    if ((t1 == integer_type || t1 == double_type) &&
        (t2 == integer_type || t2 == double_type))
      valid = 1;
    else if (t1 == boolean_type && t2 == boolean_type)
      valid = 1;

    if (!valid) {
      printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n",
             expr->line, expr->column, get_symbol_category(expr->category),
             type_to_string(t1), type_to_string(t2));
      semantic_errors++;
    }
    break;
  }

  case And:
  case Or: {
    enum type t1 = getchild(expr, 0)->type;
    enum type t2 = getchild(expr, 1)->type;

    expr->type = boolean_type;

    if (t1 != boolean_type || t2 != boolean_type) {
      printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n",
             expr->line, expr->column, get_symbol_category(expr->category),
             type_to_string(t1), type_to_string(t2));
      semantic_errors++;
    }

    break;
  }

  case Xor:
  case Lshift:
  case Rshift: {
    enum type t1 = getchild(expr, 0)->type;
    enum type t2 = getchild(expr, 1)->type;

    expr->type = integer_type;

    if (t1 != integer_type || t2 != integer_type) {
      printf("Line %d, col %d: Operator %s cannot be applied to types %s, %s\n",
             expr->line, expr->column, get_symbol_category(expr->category),
             type_to_string(t1), type_to_string(t2));
      semantic_errors++;
    }
    break;
  }

  case ParseArgs: {
    struct node *id_node = getchild(expr, 0);
    struct node *index_expr = getchild(expr, 1);

    expr->type = integer_type;

    enum type t1 = id_node->type;
    enum type t2 = index_expr->type;

    if (t1 != string_array_type || t2 != integer_type) {
      printf("Line %d, col %d: Operator Integer.parseInt cannot be applied to "
             "types %s, %s\n",
             expr->line, expr->column, type_to_string(t1), type_to_string(t2));
      semantic_errors++;
    }
    break;
  }

  case Length: {
    struct node *child = getchild(expr, 0);

    expr->type = integer_type;

    enum type t = child->type;
    int valid = 0;
    if (t == string_array_type) {
      valid = 1;
    } else {
      valid = 0;
    }

    if (!valid) {
      printf("Line %d, col %d: Operator .length cannot be applied to type %s\n",
             expr->line, expr->column, type_to_string(t));
      semantic_errors++;
    }
    break;
  }

  case Call: {
    struct node *id_node = getchild(expr, 0);
    int is_compatible = 0;
    struct symbol_list *sym =
        find_correspondent_method(id_node->token, expr, &is_compatible);

    if (sym == NULL && is_compatible == 1) {
      printf("Line %d, col %d: Reference to method %s", id_node->line,
             id_node->column, id_node->token);

      printf("(");
      struct node_list const *arg_ptr = expr->children->next->next;
      while (arg_ptr != NULL) {
        printf("%s", type_to_string(arg_ptr->node->type));
        if (arg_ptr->next != NULL)
          printf(",");
        arg_ptr = arg_ptr->next;
      }
      printf(") is ambiguous\n");
      expr->type = undef_type;
      id_node->type = undef_type;
      semantic_errors++;

    } else if (sym == NULL && is_compatible == 0) {
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

    } else if (sym != NULL) {
      expr->type = sym->type;
      id_node->type = sym->type;

      char buffer[4096] = "(";

      struct node *method_header = (sym->node) ? getchild(sym->node, 0) : NULL;
      struct node *method_params =
          (method_header) ? getchild(method_header, 2) : NULL;

      if (method_params != NULL && method_params->children != NULL) {
        struct node_list *p_list = method_params->children->next;
        while (p_list != NULL) {
          struct node *param_type_node = getchild(p_list->node, 0);
          strcat(buffer,
                 type_to_string(category_type(param_type_node->category)));

          if (p_list->next != NULL) {
            strcat(buffer, ",");
          }
          p_list = p_list->next;
        }
      }
      strcat(buffer, ")");

      id_node->parameter_types_str = strdup(buffer);
    }
    break;
  }

  case Assign: {
    enum type t1 = getchild(expr, 0)->type;
    enum type t2 = getchild(expr, 1)->type;

    expr->type = t1;

    int is_primitive =
        (t1 == integer_type || t1 == double_type || t1 == boolean_type);
    int valid = 0;

    if (is_primitive) {
      if (t1 == t2)
        valid = 1;
      else if (t1 == double_type && t2 == integer_type)
        valid = 1;
    }

    if (!valid) {
      printf("Line %d, col %d: Operator = cannot be applied to types %s, %s\n",
             expr->line, expr->column, type_to_string(t1), type_to_string(t2));
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
    return "-";
  case Plus:
    return "+";
  default:
    return "";
  }
}

struct symbol_list *find_correspondent_method(char *call_identifier,
                                              struct node *call_node,
                                              int *is_compatible) {
  if (call_identifier == NULL || call_node == NULL)
    return NULL;

  struct symbol_list *curr = symbol_table;
  struct symbol_list *exact_match = NULL;
  struct symbol_list *compatible_match = NULL;
  int compatible_count = 0;

  struct node_list *args_list = call_node->children->next->next;
  int num_args_passed = count_list(call_node->children) - 2;

  while (curr != NULL) {
    if (curr->identifier != NULL &&
        strcmp(curr->identifier, call_identifier) == 0 && curr->node != NULL &&
        curr->node->category == MethodDecl && !curr->node->is_duplicate) {

      struct node *params = getchild(getchild(curr->node, 0), 2);
      int num_params_expected = count_list(params->children) - 1;

      if (num_args_passed == num_params_expected) {
        struct node_list *p_arg = args_list;
        struct node_list *p_param = params->children->next;

        int is_exact = 1;
        int local_is_compatible = 1;

        while (p_arg != NULL && p_param != NULL) {
          enum type arg_t = p_arg->node->type;
          struct node *param_type_node = getchild(p_param->node, 0);
          if (param_type_node == NULL) {
            local_is_compatible = 0;
            break;
          }
          enum type param_t = category_type(param_type_node->category);

          if (arg_t != param_t) {
            is_exact = 0;

            if (!(arg_t == integer_type && param_t == double_type)) {
              local_is_compatible = 0;
            }
          }
          p_arg = p_arg->next;
          p_param = p_param->next;
        }

        if (local_is_compatible) {
          if (is_exact) {
            exact_match = curr;
            break;
          } else {
            compatible_match = curr;
            compatible_count++;
          }
        }
      }
    }
    curr = curr->next;
  }

  if (exact_match != NULL) {
    *is_compatible = 0;
    return exact_match;
  }

  if (compatible_count > 1) {
    *is_compatible = 1;
    return NULL;
  }

  if (compatible_count == 1) {
    *is_compatible = 1;
    return compatible_match;
  }

  *is_compatible = 0;
  return NULL;
}

void check_var_decl(struct node *var_decl, struct symbol_list *local_table) {
  if (var_decl == NULL || local_table == NULL) {
    return;
  }

  struct node *type_node = getchild(var_decl, 0);
  if (type_node == NULL) {
    return;
  }
  enum type nodes_type = category_type(type_node->category);

  int i = 1;
  struct node *id_node;

  while ((id_node = getchild(var_decl, i)) != NULL) {

    if (id_node->token == NULL) {
      i++;
      continue;
    }

    if (strcmp(id_node->token, "_") == 0) {
      is_reserved_underscore(id_node);
    } else {
      if (search_symbol(local_table, id_node->token) != NULL) {
        printf("Line %d, col %d: Symbol %s already defined\n", id_node->line,
               id_node->column, id_node->token);
        semantic_errors++;
      } else {
        insert_symbol(local_table, id_node->token, nodes_type, id_node);
      }
    }
    i++;
  }
}

char *clean_token_underscores(char *token) {
  char *clean_token = malloc(strlen(token) + 1);
  if (clean_token == NULL) {
    return NULL;
  }
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
  if (parameters == NULL)
    return;
  struct node_list *p_list =
      (parameters->children != NULL) ? parameters->children->next : NULL;

  while (p_list != NULL) {
    if (p_list->node != NULL) {
      struct node *type_node = getchild(p_list->node, 0);
      struct node *id = getchild(p_list->node, 1);

      if (id != NULL && id->token != NULL && strcmp(id->token, "_") != 0) {

        if (type_node != NULL &&
            search_symbol(scope_table, id->token) == NULL) {
          struct symbol_list *s =
              insert_symbol(scope_table, id->token,
                            category_type(type_node->category), type_node);
          if (s)
            s->is_parameter = 1;
        }
      }
    }
    p_list = p_list->next;
  }
}

void check_field_decl(struct node *field_decl,
                      struct symbol_list *global_table) {
  if (field_decl == NULL || global_table == NULL ||
      field_decl->children == NULL)
    return;

  struct node *type_node = getchild(field_decl, 0);
  if (type_node == NULL)
    return;

  enum type field_type = category_type(type_node->category);

  int i = 1;
  struct node *id_node;

  while ((id_node = getchild(field_decl, i)) != NULL) {
    if (id_node->token == NULL) {
      i++;
      continue;
    }

    if (strcmp(id_node->token, "_") == 0) {
      is_reserved_underscore(id_node);
    } else {

      int is_duplicate = 0;
      struct symbol_list *curr = global_table->next;

      while (curr != NULL) {
        if (curr->identifier != NULL) {
          if (strcmp(curr->identifier, id_node->token) == 0) {
            if (curr->node != NULL && curr->node->category == FieldDecl) {
              is_duplicate = 1;
              break;
            }
          }
        }
        curr = curr->next;
      }

      if (is_duplicate) {
        printf("Line %d, col %d: Symbol %s already defined\n", id_node->line,
               id_node->column, id_node->token);
        semantic_errors++;
      } else {
        insert_symbol(global_table, id_node->token, field_type, field_decl);
      }
    }
    i++;
  }
}

// semantic analysis begins here, with the AST root node
int check_program(struct node *program) {
  if (program == NULL || program->children == NULL) {
    return semantic_errors;
  }

  struct node *class_id = getchild(program, 0);

  symbol_table = (struct symbol_list *)malloc(sizeof(struct symbol_list));
  if (symbol_table == NULL) {
    return semantic_errors;
  }
  symbol_table->next = NULL;
  symbol_table->identifier = NULL;
  symbol_table->node = NULL;
  symbol_table->type = undef_type;

  if (class_id != NULL && class_id->token != NULL) {
    is_reserved_underscore(class_id);
    symbol_table->identifier = strdup(class_id->token);
  }

  struct node_list *child = program->children;

  while ((child = child->next) != NULL) {
    if (child->node != NULL) {
      if (child->node->category == MethodDecl) {
        add_method_to_table(child->node);
      } else if (child->node->category == FieldDecl) {
        check_field_decl(child->node, symbol_table);
      }
    }
  }

  child = program->children;
  while ((child = child->next) != NULL) {
    if (child->node != NULL) {
      if (child->node->category == MethodDecl) {
        check_method_semantics(child->node);
      }
    }
  }
  return semantic_errors;
}

// insert a new symbol in the list, unless it is already there
struct symbol_list *insert_symbol(struct symbol_list *table, char *identifier,
                                  enum type type, struct node *node) {
  if (table == NULL) {
    return NULL;
  }

  struct symbol_list *new =
      (struct symbol_list *)malloc(sizeof(struct symbol_list));
  if (new == NULL) {
    return NULL;
  }
  new->identifier = (identifier != NULL) ? strdup(identifier) : NULL;
  new->type = type;
  new->is_parameter = 0;
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
  if (identifier == NULL || table == NULL) {
    return NULL;
  }
  struct symbol_list *symbol;
  for (symbol = table->next; symbol != NULL; symbol = symbol->next)
    if (symbol->identifier != NULL &&
        strcmp(symbol->identifier, identifier) == 0)
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
  case string_type:
    return "String";
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
  if (!symbol_table)
    return;

  printf("===== Class %s Symbol Table =====\n", symbol_table->identifier);

  struct symbol_list *s = symbol_table->next;
  while (s != NULL) {
    if (s->identifier) {
      if (s->node != NULL && s->node->category == MethodDecl) {
        printf("%s\t", s->identifier);
        print_method_params(s->node);
        printf("\t%s\n", type_to_string(s->type));
      } else {
        printf("%s\t\t%s\n", s->identifier, type_to_string(s->type));
      }
    }
    s = s->next;
  }
  printf("\n");

  if (!program || !program->children)
    return;

  struct node_list *child = program->children->next;
  while (child != NULL) {
    if (child->node && child->node->category == MethodDecl) {
      if (!child->node->is_duplicate && child->node->local_symbols != NULL) {
        struct node *header = getchild(child->node, 0);
        if (header) {
          struct node *id_node = getchild(header, 1);
          char *m_name =
              (id_node && id_node->token) ? id_node->token : "unknown";

          printf("===== Method %s", m_name);
          print_method_params(child->node);
          printf(" Symbol Table =====\n");

          struct symbol_list *ls = child->node->local_symbols->next;
          while (ls != NULL) {
            if (ls->identifier) {
              printf("%s\t\t%s%s\n", ls->identifier, type_to_string(ls->type),
                     ls->is_parameter ? "\tparam" : "");
            }
            ls = ls->next;
          }
          printf("\n");
        }
      }
    }
    child = child->next;
  }
}

void print_method_params(struct node *method_decl) {
  printf("(");
  if (method_decl != NULL) {
    struct node *method_header = getchild(method_decl, 0);

    if (method_header != NULL) {
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
          if (curr_param->next != NULL)
            printf(",");
          curr_param = curr_param->next;
        }
      }
    }
  }
  printf(")");
}
