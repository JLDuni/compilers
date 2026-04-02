/* START definitions section -- C code delimited by %{ ... %} and token declarations */

%{

#include <stdio.h>
#include "ast.h"

int yylex(void);
void yyerror(char *);

struct node *ast;

%}

%nonassoc THEN
%nonassoc ELSE

%token <lexeme> IDENTIFIER NATURAL DECIMAL STRLIT RESERVED BOOLLIT

%token BOOL CLASS DOUBLE ELSE IF INT PRINT PARSEINT PUBLIC RETURN STATIC STRING VOID WHILE THEN

%token AND ASSIGN ARROW COMMA DIVIDE DOTLENGTH EQ GE GT LBRACE LPAR LSQ LE LSHIFT LT 
%token MINUS MOD NE NOT OR PLUS RBRACE RPAR RSQ RSHIFT SEMICOLON STAR XOR

%type <node> Program MethodDecl MethodHeader MethodBody  OptionalFormalParams Statement
%type <node> Assignment MethodInvocation ParseArgs 
%type <node> Expr Type 

%type <node_list> declarations BodyContent VarDecl MultipleIdentifiers StatementList
%type <node_list> FormalParams ArgsList FieldDecl 

%right ASSIGN
%left OR
%left AND
%left XOR
%left EQ NE
%left LT LE GT GE
%left LSHIFT RSHIFT
%left PLUS MINUS
%left STAR DIV MOD
%right NOT UNARY_PLUS UNARY_MINUS

%left LOW
%left '+' '-'
%left '*' '/'

%union{
    char *lexeme;
    struct node_list *node_list;
    struct node *node;
}

/* START grammar rules section -- BNF grammar */

%%

Program: CLASS IDENTIFIER LBRACE declarations RBRACE                { ast = $$ = newnode(Program, NULL);
                                                                     struct node *identifier_node = newnode(Identifier, $2); 
                                                                     struct node_list *id_list = newlist(identifier_node); 
                                                                     id_list->next = $4;
                                                                     addchildren($$, id_list); }

    ;

declarations:     { $$ = NULL; } 
            |declarations MethodDecl                 { $$ = append($1, $2); }
            | declarations FieldDecl                  { if($1 == NULL) $$ = $2;
                                                        else {struct node_list *curr = $1;
                                                              while(curr->next != NULL) curr = curr->next; 
                                                              curr->next = $2;
                                                              $$ = $1; 
                                                              } 
                                                      }
            | declarations SEMICOLON                  { $$ = $1; }
            ;

FieldDecl: PUBLIC STATIC Type IDENTIFIER MultipleIdentifiers SEMICOLON      { struct node *decl = newnode(FieldDecl, NULL); 
                                                                              addchild(decl, $3); 
                                                                              addchild(decl, newnode(Identifier, $4)); 

                                                                              struct node_list *list = newlist(decl); 
                                                              
                                                                              struct node_list *curr = $5;
                                                                              while(curr != NULL) {
                                                                                struct node *current_decl = newnode(FieldDecl, NULL);
                                                                                addchild(current_decl, copy_node($3));
                                                                                addchild(current_decl, curr->node);
                                                                                list = append(list, current_decl);
                                                                                curr = curr->next;
                                                                              } 
                                                                              $$ = list; 
                                                                              }
        | error SEMICOLON                                                     { $$ = NULL; }
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

MethodBody: LBRACE BodyContent RBRACE     { $$ = newnode(MethodBody, NULL); 
                                            addchildren($$, $2); }
          ;

BodyContent:                          { $$ = NULL; }
           | BodyContent VarDecl      { if($1 == NULL) $$ = $2;
                                        else{ struct node_list *curr = $1;
                                              while(curr->next != NULL) curr = curr->next; 
                                              curr->next = $2;
                                              $$ = $1; } 
                                      }
           | BodyContent Statement    { if($2 != NULL) $$ = append($1, $2); 
                                        else $$ = $1; }
           ;

VarDecl: Type IDENTIFIER MultipleIdentifiers SEMICOLON    { struct node *decl = newnode(VarDecl, NULL); 
                                                              addchild(decl, $1); 
                                                              addchild(decl, newnode(Identifier, $2)); 
                                                              struct node_list *list = newlist(decl); 
                                                              
                                                              struct node_list *curr = $3;
                                                              while(curr != NULL) {
                                                                struct node *current_decl = newnode(VarDecl, NULL);
                                                                addchild(current_decl, copy_node($1));
                                                                addchild(current_decl, curr->node);
                                                                list = append(list, current_decl);
                                                                curr = curr->next;
                                                              } 
                                                              $$ = list;
                                                              } 
        ;

MultipleIdentifiers: { $$ = NULL; }
                   | MultipleIdentifiers COMMA IDENTIFIER   { $$ = append($1, newnode(Identifier, $3)); }
                   ;

Statement: LBRACE StatementList RBRACE      { int count = count_list($2); 
                                              if (count == 1) $$ = $2->node;
                                              else{ $$ = newnode(Block, NULL); addchildren($$, $2); } }
        ;

Statement: IF LPAR Expr RPAR Statement %prec THEN         { $$ = newnode(If, NULL); 
                                                            addchild($$, $3); 
                                                            addchild($$, $5); 
                                                            addchild($$, NULL); }
         | IF LPAR Expr RPAR Statement ELSE Statement     { $$ = newnode(If, NULL); 
                                                            addchild($$, $3); 
                                                            addchild($$, $5); 
                                                            addchild($$, $7); }

         | WHILE LPAR Expr RPAR Statement                 { $$ = newnode(While, NULL); 
                                                            addchild($$, $3); 
                                                            addchild($$, $5); }

         | RETURN SEMICOLON                               { $$ = newnode(Return, NULL); }
         | RETURN Expr SEMICOLON                         { $$ = newnode(Return, NULL); 
                                                            addchild($$, $2);}

        | MethodInvocation SEMICOLON                     { $$ =  $1; }
        | Assignment SEMICOLON                     { $$ =  $1; }
        | ParseArgs SEMICOLON                     { $$ =  $1; }
        | PRINT LPAR Expr RPAR SEMICOLON           { $$ = newnode(Print, NULL); 
                                                      addchild($$, $3); }

        | PRINT LPAR STRLIT RPAR SEMICOLON           { $$ = newnode(StrLit, NULL); 
                                                      addchild($$, newnode(StrLit, $3)); }
        | error SEMICOLON                             { $$ = NULL; }
        ;

StatementList:          { $$ = NULL; }
             | StatementList Statement    { if( $2 != NULL) $$ = append($1, $2); 
                                            else $$ = $1; }
             ;

MethodInvocation: IDENTIFIER LPAR ArgsList RPAR     { $$ = newnode(Call, NULL); 
                                                      struct node *id_node = newnode(Identifier, $1); 
                                                      struct node_list *list = newlist(id_node); 
                                                      list->next = $3;
                                                      addchildren($$, list); 
                                                      }

                | IDENTIFIER LPAR error RPAR      { $$ = NULL;}
                ;

ArgsList:     { $$ = NULL; }
        | Expr      { $$ = newlist($1); }
        |ArgsList COMMA Expr     { $$ = append($1, $3); }
        ;

OptionalFormalParams:                 { $$ = newnode(MethodParams, NULL); }
                    | FormalParams    { newnode(MethodParams, NULL); 
                                      addchildren($$, $1); }
                    ;
         
FormalParams: Type IDENTIFIER { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                struct node_list *children = newlist($1); 
                                append(children, newnode(Identifier, $2));
                                addchildren(param_declaration, children); 
                                $$ = newlist(param_declaration);
                                }
            | FormalParams COMMA Type IDENTIFIER   { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                                     struct node_list *children = newlist($3); 
                                                     append(children, newnode(Identifier, $4)); 
                                                     addchildren(param_declaration, children); 
                                                     $$ = append($1, param_declaration); }

            | STRING LSQ RSQ IDENTIFIER            { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                                     struct node_list *children = newlist(newnode(StringArray, NULL)); 
                                                     append(children, newnode(Identifier, $4)); 
                                                     addchildren(param_declaration, children); 
                                                     $$ = newlist(param_declaration); }
            ;

Assignment: IDENTIFIER ASSIGN Expr               { $$ = newnode(Assign, NULL); 
                                                    struct node_list *list = newlist(newnode(Identifier, $1)); 
                                                    append(list, $3); 
                                                    addchildren($$, list); }
           ;

ParseArgs: PARSEINT LPAR IDENTIFIER LSQ Expr RSQ RPAR     { $$ = newnode(ParseArgs, NULL); 
                                                          struct node_list *list = newlist(newnode(Identifier, $3)); 
                                                          append(list, $5); 
                                                          addchildren($$, list); }

         | PARSEINT LPAR error RPAR                       { $$ = NULL; }
         ;

Expr: Expr PLUS Expr    { $$ = newnode(Add, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr MINUS Expr   { $$ = newnode(Sub, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr STAR Expr    { $$ = newnode(Mul, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr DIV Expr     { $$ = newnode(Div, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr MOD Expr     { $$ = newnode(Mod, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr AND Expr     { $$ = newnode(And, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr OR Expr      { $$ = newnode(Or, NULL);  addchildren($$, append(newlist($1), $3)); }
    | Expr XOR Expr     { $$ = newnode(Xor, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr LSHIFT Expr  { $$ = newnode(Lshift, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr RSHIFT Expr  { $$ = newnode(Rshift, NULL); addchildren($$, append(newlist($1), $3)); }
    | Expr EQ Expr      { $$ = newnode(Eq, NULL);  addchildren($$, append(newlist($1), $3)); }
    | Expr NE Expr      { $$ = newnode(Ne, NULL);  addchildren($$, append(newlist($1), $3)); }
    | Expr GE Expr      { $$ = newnode(Ge, NULL);  addchildren($$, append(newlist($1), $3)); }
    | Expr GT Expr      { $$ = newnode(Gt, NULL);  addchildren($$, append(newlist($1), $3)); }
    | Expr LE Expr      { $$ = newnode(Le, NULL);  addchildren($$, append(newlist($1), $3)); }
    | Expr LT Expr      { $$ = newnode(Lt, NULL);  addchildren($$, append(newlist($1), $3)); }

    | MINUS Expr %prec UNARY_MINUS { $$ = newnode(Minus, NULL); addchild($$, $2); }
    | PLUS Expr %prec UNARY_PLUS   { $$ = newnode(Plus, NULL);  addchild($$, $2); }
    | NOT Expr                     { $$ = newnode(Not, NULL);   addchild($$, $2); }
    | IDENTIFIER                   { $$ = newnode(Identifier, NULL); }
    | IDENTIFIER DOTLENGTH         { $$ = newnode(Length, NULL); addchild($$, newnode(Identifier, $1)); }
    | NATURAL                      { $$ = newnode(Natural, $1); }
    | DECIMAL                      { $$ = newnode(Decimal, $1); }
    | BOOLLIT                      { $$ = newnode(Boollit, $1); }
    | MethodInvocation             { $$ = $1; }
    | Assignment                  { $$ = $1; }
    | ParseArgs                    { $$ = $1; }
    | LPAR Expr RPAR               { $$ = $2; }
    | LPAR error RPAR              { $$ = NULL; }
    ;

Type: BOOL          { $$ = newnode(Bool, NULL); }
    | INT           { $$ = newnode(Int, NULL); }
    | DOUBLE        { $$ = newnode(Double, NULL); }
    ;

%%

/* START subroutines section */


// all needed functions are collected in the .l and ast.* files
