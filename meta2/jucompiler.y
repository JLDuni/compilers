/* START definitions section -- C code delimited by %{ ... %} and token declarations */

%{

#include <stdio.h>
#include "ast.h"

int yylex(void);
void yyerror(char *);

struct node *ast;

%}

%token INTEGER DOUBLE IF THEN ELSE
%token<lexeme> IDENTIFIER NATURAL DECIMAL
%type<node> program function parameters parameter arguments expression
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

program: functions                   { ast = $$ = newnode(Program, NULL);
                                      addchildren($$, $1); }
    ;

functions: function                 { $$ = newlist($1); }
         | functions function       { $$ = append($1, $2); }

function: IDENTIFIER '(' parameters ')' '=' expression
                                    { $$ = newnode(Function, NULL);
                                      addchild($$, newnode(Identifier, $1));
                                      addchild($$, $3);
                                      addchild($$, $6); }
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
