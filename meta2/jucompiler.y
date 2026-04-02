/* START definitions section -- C code delimited by %{ ... %} and token declarations */

%{

#include <stdio.h>
#include "ast.h"

int yylex(void);
void yyerror(char *);

struct node *ast;

%}

%token <lexeme> IDENTIFIER NATURAL DECIMAL STRLIT RESERVED
%token BOOL CLASS DOUBLE ELSE IF INT PRINT PARSEINT PUBLIC RETURN STATIC STRING VOID WHILE THEN INTEGER
%token AND ASSIGN ARROW COMMA DIVIDE DOTLENGTH EQ GE GT LBRACE LPAR LSQ LE LSHIFT LT 
%token MINUS MOD NE NOT OR PLUS RBRACE RPAR RSQ RSHIFT SEMICOLON STAR XOR

%type<node> program function parameters parameter arguments expression MethodDecl MethodHeader
%type<list> functions

%left LOW
%left '+' '-'
%left '*' '/'

%union{
    char *lexeme;
    struct node_list *list;
    struct node *node;
}

/* START grammar rules section -- BNF grammar */

%%

program: CLASS IDENTIFIER LBRACE declarations RBRACE                { ast = $$ = newnode(Program, NULL);
                                                                     struct node *identifier_node = newnode(Identifier, $2); 
                                                                     struct node_list *id_list = newlist(identifier_node); 
                                                                     id_list->next = $4;
                                                                     addchildren($$, full_list); }

    ;

declarations: declarations MethodDecl                 { $$ = append($1, $2); }
            | declarations FieldDecl                  { $$ = append($1, $2); }
            | declarations SEMICOLON                  { $$ = $1}
            ;

MethodDecl: PUBLIC STATIC MethodHeader MethodBody     { $$ = newnode(MethodDecl, NULL); 
                                                      struct node_list *list = newlist($3); 
                                                      append(list, $4);
                                                      addchildren($$, list); }
;

MethodHeader: Type IDENTIFIER LPAR OptionalFormalParams RPAR { $$ = newnode(MethodHeader, NULL); 
                                                      struct node_list *list = newlist($1);
                                                      append(list, newnode(Identifier, $2)); 
                                                      append(list, $4); 
                                                      addchildren($$, list); }

            | VOID IDENTIFIER LPAR OptionalFormalParams RPAR { $$ = newnode(MethodHeader, NULL); 
                                                               struct node_list *list = newlist(newnode(Void, NULL)); 
                                                               append(list, newnode(Identifier, $2)); 
                                                               append(list, $4); 
                                                               addchildren($$, list); }
            ;

MethodBody: 

OptionalFormalParams:                 { $$ = newnode(MethodParams, NULL); }
                    | FormalParams    { newnode(FormalParams, NULL); 
                                      addchildren($$, $1); }
         
FormalParams: Type IDENTIFIER { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                struct node_list *children = newlist($1); 
                                append(children, newnode(Identifier, $2));
                                addchildren(param_declaration, children); 
                                $$ = param_declaration;
                                }
            | FormalParams COMMA TYPE IDENTIFIER   { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                                     struct node_list *children = newlist($3); 
                                                     append(children, newnode(Identifier, $2)); 
                                                     addchildren(param_declaration, children); 
                                                     $$ = append($1, param_declaration); }

            | STRING LSQ RSQ IDENTIFIER            { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                                     struct node_list *children = newlist(newnode(StringArray, NULL)); 
                                                     append(children, newnode(Identifier, $4)); 
                                                     addchildren(param_declaration, children); 
                                                     $$ = newlist(param_declaration); }
            ;


parameters: parameter               { $$ = newnode(Parameters, NULL); addchild($$, $1); }
    | parameters ',' parameter      { addchild($1, $3); $$ = $1; }
    ;

parameter: INTEGER IDENTIFIER       {$$ = newnode(Parameter, NULL); addchild($$, newnode(Integer, NULL)); }
    | DOUBLE IDENTIFIER             {$$ = newnode(Parameter, NULL); addchild($$, newnode(Double, NULL)); }
    ;

arguments: expression               { ast = $$ = newnode(Arguments, NULL); addchild($$, $1); }
    | arguments ',' expression      { addchild($1, $3); $$ = $1; }
    ;

expression: IDENTIFIER              { $$ = newnode(Identifier, $1); }
    | NATURAL                       { $$ = newnode(Natural, $1); }
    | DECIMAL                       { $$ = newnode(Decimal, $1); }
    | IDENTIFIER '(' arguments ')'  { $$ = newnode(Call, NULL); addchild($$, newnode(Identifier, $1)); addchild($$, $3); }
    | IF expression THEN expression ELSE expression  %prec LOW
                                    {$$ = newnode(If, NULL); 
                                    addchild($$, $2); 
                                    addchild($$, $4); 
                                    addchild($$, $6); }
    | expression '+' expression     {$$ = newnode(Add, NULL); 
                                    addchild($$, $1); 
                                    addchild($$, $3); }
    | expression '-' expression     {$$ = newnode(Sub, NULL); 
                                    addchild($$, $1); 
                                    addchild($$, $3); }
    | expression '*' expression     {$$ = newnode(Mul, NULL); 
                                    addchild($$, $1); 
                                    addchild($$, $3); }
    | expression '/' expression     {$$ = newnode(Div, NULL); 
                                    addchild($$, $1); 
                                    addchild($$, $3); }
    | '(' expression ')'            { $$ = $2; }  
    ;

%%

/* START subroutines section */


// all needed functions are collected in the .l and ast.* files
