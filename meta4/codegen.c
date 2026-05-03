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

int add_string_literal(char *token) {
    str_literals[str_count] = token;
    return str_count++;
}

char *get_llvm_type(struct node *type_node) {
  switch (type_node->category) {
  case Int:
    return "i32";
  case Double:
    return "double";
  case Void:
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
  case Natural:
    tmp = temporary++;
    printf("  %%%d = add i32 0, %s\n", tmp, expr->token);
    break;

  case Decimal:
    tmp = temporary++;
    printf("  %%%d = fadd double 0.0, %s\n", tmp, expr->token);
    break;

  case BoolLit:
    tmp = temporary++;
    int b_val = (strcmp(expr->token, "true") == 0) ? 1 : 0;
    printf("  %%%d = add i1 0, %d\n", tmp, b_val);
    break;

  case Identifier: {
    char *llvm_t = get_llvm_type(expr);
    char prefix = is_global_variable(expr->token) ? '@' : '%';

    tmp = temporary++;
    printf("  %%%d = load %s, %s* %c%s\n", tmp, llvm_t, llvm_t, prefix,
           expr->token);
    break;
  }

  case ParseArgs: {
    struct node *id_node = getchild(expr, 0); // "args"
    struct node *index_expr = getchild(expr, 1);

    // Lógica avançada de GEP (GetElementPtr) para aceder a args[index] e chamar
    // atoi
    tmp = temporary++;
    // ... (A implementar: carregar array e passar a @_atoi)
    break;
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

    printf("  %%%d = call %s @_%s(", tmp, ret_t, id_node->token);
    for (int j = 0; j < num_args; j++) {
      printf("%s %%%d%s", args_types[j], args_regs[j],
             (j < num_args - 1) ? ", " : "");
    }
    printf(")\n");
    break;
  }

  case Add:
  case Sub:
  case Mul:
  case Div:
  case Mod: {
    int t1 = codegen_expression(getchild(expr, 0));
    int t2 = codegen_expression(getchild(expr, 1));

    int is_double = (expr->type == double_type);
    char *llvm_t = get_llvm_type(expr);
    char *opcode;

    if (expr->category == Add)
      opcode = is_double ? "fadd" : "add";
    else if (expr->category == Sub)
      opcode = is_double ? "fsub" : "sub";
    else if (expr->category == Mul)
      opcode = is_double ? "fmul" : "mul";
    else if (expr->category == Div)
      opcode = is_double ? "fdiv" : "sdiv";
    else if (expr->category == Mod)
      opcode = is_double ? "frem" : "srem";

    tmp = temporary++;
    printf("  %%%d = %s %s %%%d, %%%d\n", tmp, opcode, llvm_t, t1, t2);
    break;
  }

  case Eq:
  case Ne:
  case Lt:
  case Gt:
  case Le:
  case Ge: {
    struct node *c1 = getchild(expr, 0);
    int t1 = codegen_expression(c1);
    int t2 = codegen_expression(getchild(expr, 1));

    int is_double = (c1->type == double_type);
    char *llvm_t = get_llvm_type(c1);
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
    else if (expr->category == Ge)
      cond = is_double ? "oge" : "sge";

    tmp = temporary++;
    printf("  %%%d = %s %s %s %%%d, %%%d\n", tmp, cmp_instr, cond, llvm_t, t1,
           t2);
    break;
  }

  default:
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
    struct node_list *curr = statement->children;
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
    struct node *id_node = getchild(statement, 0);
    struct node *expr = getchild(statement, 1);

    int val_reg = codegen_expression(expr);

    char *llvm_t = get_llvm_type(id_node);
    char prefix = is_global_variable(id_node->token) ? '@' : '%';

    printf("  store %s %%%d, %s* %c%s\n", llvm_t, val_reg, llvm_t, prefix,
           id_node->token);
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
      // TODO String logic
    } else {
      int val_reg = codegen_expression(expr);
      char *llvm_t = get_llvm_type(expr);
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
    printf("i32 %%%s", getchild(parameter, 1)->token);
  }
}

void codegen_field_decl(struct node *field_delc) {
  struct node *id_node = getchild(field_delc, 1);

  printf("@%s = global i32 0\n\n", id_node->token);
}

void codegen_method(struct node *method_decl) {
  temporary = 1;

  struct node *method_header = getchild(method_decl, 0);
  struct node *method_body = getchild(method_decl, 1);

  struct node *type = getchild(method_header, 0);
  struct node *id_node = getchild(method_header, 1);
  struct node *params = getchild(method_header, 2);

  char *return_type = get_llvm_type(type);
  printf("define %s @_%s(", return_type, id_node);
  codegen_parameters(params);
  printf(") {\n");

  int last_temporary = codegen_expression(method_body);

  if (strcmp(return_type, "void") == 0) {
    printf("  ret void\n");
  } else if (last_temporary != -1) {
    printf("ret %s %%%d\n", return_type, last_temporary);
  } else {
    printf("  ret %s %s\n", return_type,
           (type->category == Double) ? "0.0" : "0");
  }

  printf("}\n\n");
}

// code generation begins here, with the AST root node
void codegen_program(struct node *program) {
  printf("declare i32 @_read(i32)\n");
  printf("declare i32 @_write(i32)\n\n");

  struct node_list *current = program->children;
  while (current != NULL) {
    struct node *child = current->node;

    if (child->category == FieldDecl) {
      codegen_field_decl(child);
    } else if (child->category == MethodDecl) {
      codegen_method(child);
    }

    current = current->next;
  }

  struct symbol_list *entry = search_symbol(symbol_table, "main");
  if (entry != NULL && (entry->node->category == MethodDecl)) {
    printf("define i32 @main() {\n");
    printf("  %%1 = call i32 @_main(i32 0)\n");
    printf("  ret i32 %%1\n");
    printf("}\n");
  }
}
