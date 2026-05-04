#include "codegen.h"
#include "ast.h"
#include "semantics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int temporary; // sequence of temporary registers in a function
int label_count;

extern struct symbol_list *symbol_table;

char *str_literals[1000];
int str_count = 0;

const char *get_mangling_type(enum type t) {
  if (t == integer_type)
    return "_int";
  if (t == double_type)
    return "_double";
  if (t == boolean_type)
    return "_boolean";
  return ""; // Void ou string array (o main) não precisa
}

int get_string_length(char *str) {
  int len = 1;
  int start = (str[0] == '"') ? 1 : 0;
  int end = (str[strlen(str) - 1] == '"') ? strlen(str) - 1 : strlen(str);

  for (int i = start; i < end; i++) {
    if (str[i] == '\\' &&
        (str[i + 1] == 'n' || str[i + 1] == 't' || str[i + 1] == '"' ||
         str[i + 1] == '\\' || str[i + 1] == 'f' || str[i + 1] == 'r')) {
      i++;
    }
    len++;
  }
  return len;
}

int convert_to_double(int reg, enum type current_t, enum type target_t) {
  if (current_t == integer_type && target_t == double_type) {
    int new_reg = temporary++;
    printf("  %%%d = sitofp i32 %%%d to double\n", new_reg, reg);
    return new_reg;
  }
  return reg;
}

char *get_binary_opcode(category cat, int is_double) {
  switch (cat) {
  case Add:
    return is_double ? "fadd" : "add";
  case Sub:
    return is_double ? "fsub" : "sub";
  case Mul:
    return is_double ? "fmul" : "mul";
  case Div:
    return is_double ? "fdiv" : "sdiv";
  case Mod:
    return is_double ? "frem" : "srem";
  default:
    return "";
  }
}

int add_string_literal(char *token) {
  str_literals[str_count] = token;
  return str_count++;
}

char *get_llvm_type(struct node *n) {
  switch (n->category) {
  case Int:
    return "i32";
  case Double:
    return "double";
  case Bool:
    return "i1";
  case Void:
    return "void";
  case StringArray:
    return "i8**";
  default:
    break;
  }

  switch (n->type) {
  case integer_type:
    return "i32";
  case double_type:
    return "double";
  case boolean_type:
    return "i1";
  case void_type:
    return "void";
  default:
    return "i32";
  }
}

int is_global_variable(char *identifier) {
  struct symbol_list *sym = search_symbol(symbol_table, identifier);
  if (sym != NULL && sym->node != NULL && sym->node->category == FieldDecl) {
    return 1;
  }
  return 0;
}

int codegen_expression(struct node *expr) {
  if (expr == NULL)
    return -1;
  int tmp = -1;

  switch (expr->category) {
  case Natural: {
    char clean_str[1024];
    int idx = 0;

    for (int i = 0; expr->token[i] != '\0'; i++) {
      if (expr->token[i] != '_') {
        clean_str[idx++] = expr->token[i];
      }
    }
    clean_str[idx] = '\0';

    tmp = temporary++;
    printf("  %%%d = add i32 0, %s\n", tmp, clean_str);
    return tmp;
  }

  case Decimal: {
    char clean_str[1024];
    int idx = 0;

    for (int i = 0; expr->token[i] != '\0'; i++) {
      if (expr->token[i] != '_') {
        clean_str[idx++] = expr->token[i];
      }
    }
    clean_str[idx] = '\0';

    tmp = temporary++;
    printf("  %%%d = fadd double %.16e, 0.0\n", tmp, atof(clean_str));
    return tmp;
  }

  case BoolLit:
    tmp = temporary++;
    printf("  %%%d = add i1 %d, 0\n", tmp,
           strcmp(expr->token, "true") == 0 ? 1 : 0);
    return tmp;

  case Identifier: {
    char *llvm_t = get_llvm_type(expr);
    char prefix = is_global_variable(expr->token) ? '@' : '%';

    tmp = temporary++;
    printf("  %%%d = load %s, %s* %c%s\n", tmp, llvm_t, llvm_t, prefix,
           expr->token);
    break;
  }

  case Xor: {
    int t1 = codegen_expression(getchild(expr, 0));
    int t2 = codegen_expression(getchild(expr, 1));
    tmp = temporary++;
    printf("  %%%d = xor i32 %%%d, %%%d\n", tmp, t1, t2);
    return tmp;
  }

  case Lshift: {
    int t1 = codegen_expression(getchild(expr, 0));
    int t2 = codegen_expression(getchild(expr, 1));
    tmp = temporary++;
    printf("  %%%d = shl i32 %%%d, %%%d\n", tmp, t1, t2);
    return tmp;
  }

  case Assign: {
    struct node *id_node = getchild(expr, 0);
    struct node *expr_node = getchild(expr, 1);
    int val_reg = codegen_expression(expr_node);

    if (id_node->type == double_type && expr_node->type == integer_type) {
      int cast_reg = temporary++;
      printf("  %%%d = sitofp i32 %%%d to double\n", cast_reg, val_reg);
      val_reg = cast_reg;
    }

    char prefix = is_global_variable(id_node->token) ? '@' : '%';
    char *llvm_t = get_llvm_type(id_node);

    printf("  store %s %%%d, %s* %c%s\n", llvm_t, val_reg, llvm_t, prefix,
           id_node->token);

    return val_reg;
  }

  case And: {
    int id = label_count++;
    int res_ptr = temporary++;
    printf("  %%%d = alloca i1\n", res_ptr);

    int t1 = codegen_expression(getchild(expr, 0));
    printf("  store i1 %%%d, i1* %%%d\n", t1, res_ptr);

    printf("  br i1 %%%d, label %%L%d_eval_right, label %%L%d_end\n", t1, id,
           id);

    printf("L%d_eval_right:\n", id);
    int t2 = codegen_expression(getchild(expr, 1));
    printf("  store i1 %%%d, i1* %%%d\n", t2, res_ptr);
    printf("  br label %%L%d_end\n", id);

    printf("L%d_end:\n", id);
    tmp = temporary++;
    printf("  %%%d = load i1, i1* %%%d\n", tmp, res_ptr);
    return tmp;
  }

  case Or: {
    int id = label_count++;
    int res_ptr = temporary++;
    printf("  %%%d = alloca i1\n", res_ptr);

    int t1 = codegen_expression(getchild(expr, 0));
    printf("  store i1 %%%d, i1* %%%d\n", t1, res_ptr);

    printf("  br i1 %%%d, label %%L%d_end, label %%L%d_eval_right\n", t1, id,
           id);

    printf("L%d_eval_right:\n", id);
    int t2 = codegen_expression(getchild(expr, 1));
    printf("  store i1 %%%d, i1* %%%d\n", t2, res_ptr);
    printf("  br label %%L%d_end\n", id);

    printf("L%d_end:\n", id);
    tmp = temporary++;
    printf("  %%%d = load i1, i1* %%%d\n", tmp, res_ptr);
    return tmp;
  }

  case Rshift: {
    int t2 = codegen_expression(getchild(expr, 1));
    int t1 = codegen_expression(getchild(expr, 0));
    tmp = temporary++;
    printf("  %%%d = ashr i32 %%%d, %%%d\n", tmp, t1, t2);
    return tmp;
  }

  case Not: {
    int v = codegen_expression(getchild(expr, 0));
    tmp = temporary++;
    printf("  %%%d = xor i1 %%%d, 1\n", tmp, v);
    return tmp;
  }

  case Plus:
    return codegen_expression(getchild(expr, 0));

  case Minus: {
    int v = codegen_expression(getchild(expr, 0));
    tmp = temporary++;
    if (expr->type == double_type)
      printf("  %%%d = fneg double %%%d\n", tmp, v);
    else
      printf("  %%%d = sub i32 0, %%%d\n", tmp, v);
    return tmp;
  }

  case ParseArgs: {
    struct node *id_node = getchild(expr, 0);
    struct node *index_expr = getchild(expr, 1);

    int base_reg = codegen_expression(id_node);

    int idx_reg = codegen_expression(index_expr);

    int gep_reg = temporary++;
    printf("  %%%d = getelementptr inbounds i8*, i8** %%%d, i32 %%%d\n",
           gep_reg, base_reg, idx_reg);

    int str_reg = temporary++;
    printf("  %%%d = load i8*, i8** %%%d\n", str_reg, gep_reg);

    tmp = temporary++;
    printf("  %%%d = call i32 @atoi(i8* %%%d)\n", tmp, str_reg);

    break;
  }

  case Length: {
    int tmp_reg = temporary++;

    printf("  %%%d = load i32, i32* @.args_length\n", tmp_reg);

    return tmp_reg;
  }

  case Call: {
    struct node *id_node = getchild(expr, 0);
    struct node *args_list = getchild(expr, 1);

    int args_regs[100];
    char *args_types[100];
    int num_args = 0;

    struct node *arg;
    int i = 0;
    if (args_list != NULL) {
      while ((arg = getchild(args_list, i++)) != NULL) {
        args_regs[num_args] = codegen_expression(arg);
        args_types[num_args] = get_llvm_type(arg);
        num_args++;
      }
    }

    char *ret_t = get_llvm_type(expr);
    tmp = temporary++;

    if (strcmp(ret_t, "void") == 0) {
      printf("  call void @_%s(", id_node->token);
      for (int j = 0; j < num_args; j++) {
        printf("%s %%%d%s", args_types[j], args_regs[j],
               (j < num_args - 1) ? ", " : "");
      }
      printf(")\n");
      return -1; // Muito importante: não gastar um temporary
    } else {
      tmp = temporary++;
      printf("  %%%d = call %s @_%s(", tmp, ret_t, id_node->token);
      for (int j = 0; j < num_args; j++) {
        printf("%s %%%d%s", args_types[j], args_regs[j],
               (j < num_args - 1) ? ", " : "");
      }
      printf(")\n");
      break;
    }
    break;
  }

  case Add:
  case Sub:
  case Mul:
  case Div:
  case Mod: {
    struct node *left = getchild(expr, 0);
    struct node *right = getchild(expr, 1);

    int t1 = codegen_expression(left);
    int t2 = codegen_expression(right);

    t1 = convert_to_double(t1, left->type, expr->type);
    t2 = convert_to_double(t2, right->type, expr->type);

    int is_double = (expr->type == double_type);
    char *opcode = get_binary_opcode(expr->category, is_double);
    char *llvm_t = is_double ? "double" : "i32";

    tmp = temporary++;
    printf("  %%%d = %s %s %%%d, %%%d\n", tmp, opcode, llvm_t, t1, t2);
    return tmp;
  }

  case Eq:
  case Ne:
  case Lt:
  case Gt:
  case Le:
  case Ge: {
    struct node *left = getchild(expr, 0);
    struct node *right = getchild(expr, 1);

    int t1 = codegen_expression(left);
    int t2 = codegen_expression(right);

    enum type compare_type =
        (left->type == double_type || right->type == double_type)
            ? double_type
            : integer_type;

    t1 = convert_to_double(t1, left->type, compare_type);
    t2 = convert_to_double(t2, right->type, compare_type);

    int is_double = (compare_type == double_type);
    char *cmp_instr = is_double ? "fcmp" : "icmp";
    char *cond;

    if (expr->category == Eq)
      cond = is_double ? "oeq" : "eq";
    else if (expr->category == Ne)
      cond = is_double ? "one" : "ne";
    else if (expr->category == Lt)
      cond = is_double ? "olt" : "slt";
    else if (expr->category == Le)
      cond = is_double ? "ole" : "sle";
    else if (expr->category == Gt)
      cond = is_double ? "ogt" : "sgt";
    else
      cond = is_double ? "oge" : "sge";

    tmp = temporary++;
    printf("  %%%d = %s %s %s %%%d, %%%d\n", tmp, cmp_instr, cond,
           is_double ? "double" : "i32", t1, t2);
    return tmp;
  }

  default:
    printf("Node not treated category: %d\n", expr->category);
    break;
  }

  return tmp;
}

void codegen_statement(struct node *statement) {
  if (statement == NULL)
    return;

  switch (statement->category) {
  case MethodBody:
  case Block: {
    struct node_list *curr = statement->children->next;
    while (curr != NULL) {
      codegen_statement(curr->node);
      curr = curr->next;
    }
    break;
  }

  case VarDecl: {
    struct node *type_node = getchild(statement, 0);
    char *llvm_type = get_llvm_type(type_node);

    int i = 1;
    struct node *id_node;
    while ((id_node = getchild(statement, i++)) != NULL) {
      printf("  %%%s = alloca %s\n", id_node->token, llvm_type);
    }
    break;
  }

  case Assign: {
    codegen_expression(statement);
    break;
  }

  case If: {
    int id = label_count++;
    int cond_reg = codegen_expression(getchild(statement, 0));

    printf("  br i1 %%%d, label %%L%d_then, label %%L%d_else\n", cond_reg, id,
           id);

    printf("L%d_then:\n", id);
    codegen_statement(getchild(statement, 1));
    printf("  br label %%L%d_end\n", id);

    printf("L%d_else:\n", id);
    if (getchild(statement, 2) != NULL) {
      codegen_statement(getchild(statement, 2));
    }
    printf("  br label %%L%d_end\n", id);

    printf("L%d_end:\n", id);
    break;
  }

  case While: {
    int id = label_count++;
    printf("  br label %%L%d_cond\n", id);

    printf("L%d_cond:\n", id);
    int cond_reg = codegen_expression(getchild(statement, 0));
    printf("  br i1 %%%d, label %%L%d_body, label %%L%d_end\n", cond_reg, id,
           id);

    printf("L%d_body:\n", id);
    codegen_statement(getchild(statement, 1));
    printf("  br label %%L%d_cond\n", id);

    printf("L%d_end:\n", id);
    break;
  }

  case Return: {
    struct node *expr = getchild(statement, 0);
    if (expr != NULL) {
      int val_reg = codegen_expression(expr);
      char *llvm_t = get_llvm_type(expr);
      printf("  ret %s %%%d\n", llvm_t, val_reg);
    } else {
      printf("  ret void\n");
    }
    break;
  }

  case Print: {
    struct node *expr = getchild(statement, 0);
    if (expr->category == StrLit) {
      int str_id = add_string_literal(expr->token);
      int len = get_string_length(expr->token);

      int p_reg = temporary++;
      printf("  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
             "([3 x i8], [3 x i8]* @.fmt_str, i32 0, i32 0), i8* getelementptr "
             "inbounds ([%d x i8], [%d x i8]* @.str.%d, i32 0, i32 0))\n",
             p_reg, len, len, str_id);
    } else {
      int temporary_register = codegen_expression(expr);
      enum type type = expr->type;

      if (type == integer_type) {
        int p_reg = temporary++;
        printf(
            "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
            "([3 x i8], [3 x i8]* @.fmt_int, i32 0, i32 0), i32 %%%d)\n",
            p_reg, temporary_register);
      } else if (type == double_type) {
        int p_reg = temporary++;
        printf(
            "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
            "([6 x i8], [6 x i8]* @.fmt_double, i32 0, i32 0), double %%%d)\n",
            p_reg, temporary_register);
      } else if (type == boolean_type) {
        int str_ptr_reg = temporary++;
        printf("  %%%d = select i1 %%%d, i8* getelementptr inbounds ([5 x i8], "
               "[5 x i8]* @.fmt_true, i32 0, i32 0), i8* getelementptr "
               "inbounds ([6 x i8], [6 x i8]* @.fmt_false, i32 0, i32 0)\n",
               str_ptr_reg, temporary_register);

        int p_reg = temporary++;
        printf(
            "  %%%d = call i32 (i8*, ...) @printf(i8* getelementptr inbounds "
            "([3 x i8], [3 x i8]* @.fmt_str, i32 0, i32 0), i8* %%%d)\n",
            p_reg, str_ptr_reg);
      }
    }
    break;
  }

  case Call:
  case ParseArgs:
    codegen_expression(statement);
    break;

  default:
    break;
  }
}

void codegen_parameters(struct node *parameters) {
  struct node *parameter;
  int curr = 0;
  while ((parameter = getchild(parameters, curr++)) != NULL) {
    if (curr > 1)
      printf(", ");

    struct node *type_node = getchild(parameter, 0);
    struct node *id_node = getchild(parameter, 1);

    printf("%s %%%s.reg", get_llvm_type(type_node), id_node->token);
  }
}

void codegen_field_decl(struct node *field_decl) {
  struct node *type_node = getchild(field_decl, 0);
  struct node *id_node = getchild(field_decl, 1);

  char *llvm_type = get_llvm_type(type_node);
  char *init_val = (type_node->category == Double) ? "0.0" : "0";

  printf("@%s = global %s %s\n\n", id_node->token, llvm_type, init_val);
}

void codegen_method(struct node *method_decl) {
  temporary = 1;

  struct node *method_header = getchild(method_decl, 0);
  struct node *method_body = getchild(method_decl, 1);

  struct node *type = getchild(method_header, 0);
  struct node *id_node = getchild(method_header, 1);
  struct node *params = getchild(method_header, 2);

  char *return_type = get_llvm_type(type);
  bool is_real_main = false;
  if (strcmp(id_node->token, "main") == 0 &&
      count_list(params->children) == 1) {
    struct node *param_decl = getchild(params, 0);
    struct node *type_node = getchild(param_decl, 0);
    if (type_node->category == StringArray) {
      is_real_main = true;
    }
  }

  char mangled_name[256];
  strcpy(mangled_name, id_node->token);

  if (!is_real_main) {
    if (params != NULL && params->children != NULL) {
      struct node_list *curr_param = params->children->next;
      while (curr_param != NULL) {
        struct node *param_decl = curr_param->node;
        enum type type = category_type(getchild(param_decl, 0)->category);

        strcat(mangled_name, get_mangling_type(type));

        curr_param = curr_param->next;
      }
    }
  }

  printf("define %s @_%s(", return_type, mangled_name);
  codegen_parameters(params);
  printf(") {\n");

  if (params != NULL && params->children != NULL) {
    struct node_list *curr_p = params->children->next;
    while (curr_p != NULL) {
      struct node *p_decl = curr_p->node;
      char *p_name = getchild(p_decl, 1)->token;
      char *p_llvm_type = get_llvm_type(getchild(p_decl, 0));

      printf("  %%%s = alloca %s\n", p_name, p_llvm_type);
      printf("  store %s %%%s.reg, %s* %%%s\n", p_llvm_type, p_name,
             p_llvm_type, p_name);

      curr_p = curr_p->next;
    }
  }

  if (method_body != NULL && method_body->children != NULL) {
    struct node_list *curr = method_body->children->next;
    while (curr != NULL) {
      codegen_statement(curr->node);
      curr = curr->next;
    }
  }

  if (strcmp(return_type, "void") == 0) {
    printf("  ret void\n");
  } else {
    printf("  ret %s %s\n", return_type,
           (type->category == Double) ? "0.0" : "0");
  }

  printf("}\n\n");
}

// code generation begins here, with the AST root node
void codegen_program(struct node *program) {
  printf("declare i32 @printf(i8*, ...)\n\n");
  printf("declare i32 @atoi(i8*)\n");
  printf("@.args_length = global i32 0\n");

  printf("@.fmt_int = private unnamed_addr constant [3 x i8] c\"%%d\\00\"\n");
  printf("@.fmt_double = private unnamed_addr constant [6 x i8] "
         "c\"%%.16e\\00\"\n");
  printf("@.fmt_str = private unnamed_addr constant [3 x i8] c\"%%s\\00\"\n");
  printf("@.fmt_true = private unnamed_addr constant [5 x i8] c\"true\\00\"\n");
  printf("@.fmt_false = private unnamed_addr constant [6 x i8] "
         "c\"false\\00\"\n\n");

  struct node_list *current = program->children->next;
  while (current != NULL) {
    struct node *child = current->node;

    if (child->category == FieldDecl) {
      codegen_field_decl(child);
    } else if (child->category == MethodDecl) {
      codegen_method(child);
    }

    current = current->next;
  }

  for (int i = 0; i < str_count; i++) {
    char *str = str_literals[i];
    char buffer[1024] = "";
    int real_len = get_string_length(str);

    int start = (str[0] == '"') ? 1 : 0;
    int end = (str[strlen(str) - 1] == '"') ? strlen(str) - 1 : strlen(str);

    for (int j = start; j < end; j++) {
      if (str[j] == '\\' && str[j + 1] == 'n') {
        strcat(buffer, "\\0A");
        j++;
      } else if (str[j] == '\\' && str[j + 1] == 't') {
        strcat(buffer, "\\09");
        j++;
      } else if (str[j] == '\\' && str[j + 1] == '"') {
        strcat(buffer, "\\22");
        j++;
      } else if (str[j] == '\\' && str[j + 1] == '\\') {
        strcat(buffer, "\\5C");
        j++;
      } else if (str[j] == '\\' && str[j + 1] == 'f') {
        strcat(buffer, "\\0C");
        j++;
      } else if (str[j] == '\\' && str[j + 1] == 'r') {
        strcat(buffer, "\\0A");
        j++;
      } else {
        int pos = strlen(buffer);
        buffer[pos] = str[j];
        buffer[pos + 1] = '\0';
      }
    }
    printf("@.str.%d = private unnamed_addr constant [%d x i8] c\"%s\\00\"\n",
           i, real_len, buffer);
  }

  struct symbol_list *entry = search_symbol(symbol_table, "main");
  if (entry != NULL && (entry->node->category == MethodDecl)) {
    printf("define i32 @main(i32 %%argc, i8** %%argv) {\n");
    printf("  %%actual_argc = sub i32 %%argc, 1\n");
    printf("  store i32 %%actual_argc, i32* @.args_length\n");
    printf("  call void @_main(i8** %%argv)\n");
    printf("  ret i32 0\n");
    printf("}\n");
  }
}
