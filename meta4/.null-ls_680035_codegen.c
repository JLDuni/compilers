#include "codegen.h"
#include "ast.h"
#include "semantics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int temporary; // sequence of temporary registers in a function
int label_count;

extern struct symbol_list *symbol_table;

int codegen_natural(struct node *natural) {
  printf("  %%%d = add i32 %s, 0\n", temporary, natural->token);
  return temporary++;
}

int codegen_identifier(struct node *identifier) {
  printf("  %%%d = add i32 %%%s, 0\n", temporary, identifier->token);
  return temporary++;
}

int codegen_expression(struct node *expression) {
  int tmp = -1;
  switch (expression->category) {
  case Natural:
    tmp = codegen_natural(expression);
    break;
  case Identifier:
    tmp = codegen_identifier(expression);
    break;

  case Div:
  case Add:
  case Sub:
  case Mul:
    int t1 = codegen_expression(getchild(expression, 0));
    int t2 = codegen_expression(getchild(expression, 1));

    char *opcode;
    if (expression->category == Add)
      opcode = "add";
    else if (expression->category == Sub)
      opcode = "sub";
    else if (expression->category == Mul)
      opcode = "mul";
    else
      opcode = "sdiv";

    tmp = temporary;

    printf("  %%%d = %s i32 %%%d, %%%d\n", temporary, opcode, t1, t2);

    temporary++;

    break;

  case Call: {
    struct node *id_node = getchild(expression, 0);
    struct node *args_list =
        getchild(expression, 1); 

    int args_regs[100];
    int num_args = 0;
    struct node *arg_node;
    int j = 0;

    if (args_list != NULL) {
      while ((arg_node = getchild(args_list, j++)) != NULL) {
        args_regs[num_args++] = codegen_expression(arg_node);
      }
    }

    tmp = temporary++;
    printf("  %%%d = call i32 @_%s(", tmp, id_node->token);
    for (int k = 0; k < num_args; k++) {
      printf("i32 %%%d%s", args_regs[k], (k < num_args - 1) ? ", " : "");
    }
    printf(")\n");
    break;
  }

  case If: {
    int id = label_count++;

    int res_ptr = temporary++;
    printf("  %%%d = alloca i32\n", res_ptr);

    int cond_reg = codegen_expression(getchild(expression, 0));

    int test_reg = temporary++;
    printf("  %%%d = icmp ne i32 %%%d, 0\n", test_reg, cond_reg);

    printf("  br i1 %%%d, label %%L%dthen, label %%L%delse\n", test_reg, id,
           id);

    printf("L%dthen:\n", id);
    int then_val = codegen_expression(getchild(expression, 1));
    printf("  store i32 %%%d, i32* %%%d\n", then_val,
           res_ptr); // Guarda o resultado
    printf("  br label %%L%dend\n", id);

    printf("L%delse:\n", id);
    int else_val = codegen_expression(getchild(expression, 2));
    printf("  store i32 %%%d, i32* %%%d\n", else_val,
           res_ptr); // Guarda o resultado
    printf("  br label %%L%dend\n", id);

    printf("L%dend:\n", id);
    tmp = temporary++;
    printf("  %%%d = load i32, i32* %%%d\n", tmp, res_ptr);

    break;
  }
  }

  return tmp;
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

void codegen_function(struct node *function) {
  temporary = 1;
  printf("define i32 @_%s(", getchild(function, 0)->token);
  codegen_parameters(getchild(function, 1));
  printf(") {\n");
  int tmp = codegen_expression(getchild(function, 2));
  printf("  ret i32 %%%d\n", tmp);
  printf("}\n\n");
}

// code generation begins here, with the AST root node
void codegen_program(struct node *program) {
  // predeclared I/O functions
  printf("declare i32 @_read(i32)\n");
  printf("declare i32 @_write(i32)\n\n");

  // generate code for each function
  struct node_list *function = program->children;
  while ((function = function->next) != NULL)
    codegen_function(function->node);

  // generate the entry point which calls main(integer) if it exists
  struct symbol_list *entry = search_symbol(symbol_table, "main");
  if (entry != NULL && entry->node->category == Function)
    printf("define i32 @main() {\n"
           "  %%1 = call i32 @_main(i32 0)\n"
           "  ret i32 %%1\n"
           "}\n");
}
