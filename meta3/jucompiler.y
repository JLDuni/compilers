/* START definitions section -- C code delimited by %{ ... %} and token declarations */

%{

#include <stdio.h>
#include "ast.h"

int yylex(void);
void yyerror(char *);

struct node *ast;

%}

%union{
    char *lexeme;
    struct node_list *node_list;
    struct node *node;
}


%token <lexeme> IDENTIFIER NATURAL DECIMAL STRLIT RESERVED BOOLLIT

%token BOOL CLASS DOUBLE ELSE IF INT PRINT PARSEINT PUBLIC RETURN STATIC STRING VOID WHILE
%token AND ASSIGN ARROW COMMA DIV DOTLENGTH EQ GE GT LBRACE LPAR LSQ LE LSHIFT LT 
%token MINUS MOD NE NOT OR PLUS RBRACE RPAR RSQ RSHIFT SEMICOLON STAR XOR

%type <node> Program MethodDecl MethodHeader MethodBody  OptionalFormalParams Statement
%type <node> Assignment MethodInvocation ParseArgs 
%type <node> Expr Type OperationExpr

%type <node_list> declarations BodyContent VarDecl MultipleIdentifiers StatementList
%type <node_list> FormalParams ArgsList FieldDecl NonEmptyArgsList TypeList

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
%left LPAR RPAR LSQ RSQ DOTLENGTH 

%nonassoc IF_PREC
%nonassoc ELSE

/* START grammar rules section -- BNF grammar */

%%

Program: CLASS IDENTIFIER LBRACE declarations RBRACE                { ast = $$ = newnode(Program, NULL);
                                                                     struct node *identifier_node = newnode(Identifier, $2); 
                                                                     struct node_list *id_list = newlist(identifier_node); 
                                                                     id_list->next = $4;
                                                                     addchildren($$, id_list); }

    ;

declarations:     { $$ = NULL; } 
            | declarations MethodDecl                 { $$ = append($1, $2); }
            | declarations FieldDecl                  { if($1 == NULL) $$ = $2;
                                                        else {struct node_list *curr = $1;
                                                              while(curr->next != NULL) curr = curr->next; 
                                                              curr->next = $2;
                                                              $$ = $1; 
                                                              } 
                                                      }
            | declarations SEMICOLON                  { $$ = $1; }
            // | declarations error SEMICOLON            { $$ = $1; }
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
         | error SEMICOLON                                                    {$$ = NULL; }
        ;

MethodDecl: PUBLIC STATIC MethodHeader MethodBody     { $$ = newnode(MethodDecl, NULL); 
                                                      struct node_list *list = newlist($3); 
                                                      append(list, $4);
                                                      addchildren($$, list); }
;

MethodHeader: Type IDENTIFIER LPAR OptionalFormalParams RPAR { $$ = newnode(MethodHeader, NULL); 
                                                      struct node_list *list = newlist($1);
                                                      append(list, newnode(Identifier, $2)); 
                                                      list = append(list, $4); 
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
                                              if (count > 1) {$$ = newnode(Block, NULL); addchildren($$, $2);}
                                              else if (count == 1) $$ = $2->node;
                                              else{ $$ = NULL; } }
        ;

Statement: IF LPAR Expr RPAR Statement %prec IF_PREC         { $$ = newnode(If, NULL); 
                                                            addchild($$, $3); 
                                                            addchild($$, $5 != NULL ? $5 : newnode(Block, NULL)); 
                                                            addchild($$, newnode(Block, NULL)); }
         | IF LPAR Expr RPAR Statement ELSE Statement     { $$ = newnode(If, NULL); 
                                                            addchild($$, $3); 
                                                            addchild($$, $5 != NULL ? $5 : newnode(Block, NULL)); 
                                                            addchild($$, $7 != NULL ? $7 : newnode(Block, NULL)); }

         | WHILE LPAR Expr RPAR Statement                 { $$ = newnode(While, NULL); 
                                                            addchild($$, $3); 
                                                            addchild($$, $5 != NULL ? $5 : newnode(Block, NULL)); }

         | RETURN SEMICOLON                               { $$ = newnode(Return, NULL); }
         | RETURN Expr SEMICOLON                         { $$ = newnode(Return, NULL); 
                                                            addchild($$, $2);}

        | MethodInvocation SEMICOLON                     { $$ =  $1; }
        | Assignment SEMICOLON                     { $$ =  $1; }
        | ParseArgs SEMICOLON                     { $$ =  $1; }
        | PRINT LPAR Expr RPAR SEMICOLON           { $$ = newnode(Print, NULL); 
                                                      addchild($$, $3); }

        | PRINT LPAR STRLIT RPAR SEMICOLON           { $$ = newnode(Print, NULL); 
                                                      addchild($$, newnode(StrLit, $3)); }
        | error SEMICOLON                             { $$ = NULL; }
        | SEMICOLON                                   { $$ = NULL; }
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
        | NonEmptyArgsList  { $$ = $1; }
        ;

NonEmptyArgsList: Expr      { $$ = newlist($1); }
                | NonEmptyArgsList COMMA Expr     { $$ = append($1, $3); }
                ;

OptionalFormalParams:                 { $$ = newnode(MethodParams, NULL); }
                    | FormalParams    { $$ = newnode(MethodParams, NULL); 
                                      addchildren($$, $1); }
                    ;
         
FormalParams: TypeList                              { $$ = $1; }             
            | STRING LSQ RSQ IDENTIFIER            { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                                     struct node_list *children = newlist(newnode(StringArray, NULL)); 
                                                     append(children, newnode(Identifier, $4)); 
                                                     addchildren(param_declaration, children); 
                                                     $$ = newlist(param_declaration); }

            ;

TypeList: Type IDENTIFIER { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                struct node_list *children = newlist($1); 
                                append(children, newnode(Identifier, $2));
                                addchildren(param_declaration, children); 
                                $$ = newlist(param_declaration);
                                }
            | TypeList COMMA Type IDENTIFIER   { struct node *param_declaration = newnode(ParamDecl, NULL); 
                                                     struct node_list *children = newlist($3); 
                                                     append(children, newnode(Identifier, $4)); 
                                                     addchildren(param_declaration, children); 
                                                     $$ = append($1, param_declaration); }

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

Expr: Assignment        { $$ = $1; }
    | OperationExpr     { $$ = $1; }

OperationExpr: OperationExpr PLUS OperationExpr    { $$ = newnode(Add, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr MINUS OperationExpr   { $$ = newnode(Sub, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr STAR OperationExpr    { $$ = newnode(Mul, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr DIV OperationExpr     { $$ = newnode(Div, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr MOD OperationExpr     { $$ = newnode(Mod, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr AND OperationExpr     { $$ = newnode(And, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr OR OperationExpr      { $$ = newnode(Or, NULL);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr XOR OperationExpr     { $$ = newnode(Xor, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr LSHIFT OperationExpr  { $$ = newnode(Lshift, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr RSHIFT OperationExpr  { $$ = newnode(Rshift, NULL); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr EQ OperationExpr      { $$ = newnode(Eq, NULL);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr NE OperationExpr      { $$ = newnode(Ne, NULL);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr GE OperationExpr      { $$ = newnode(Ge, NULL);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr GT OperationExpr      { $$ = newnode(Gt, NULL);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr LE OperationExpr      { $$ = newnode(Le, NULL);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr LT OperationExpr      { $$ = newnode(Lt, NULL);  addchildren($$, append(newlist($1), $3)); }

    | MINUS OperationExpr %prec UNARY_MINUS { $$ = newnode(Minus, NULL); addchild($$, $2); }
    | PLUS OperationExpr %prec UNARY_PLUS   { $$ = newnode(Plus, NULL);  addchild($$, $2); }
    | NOT OperationExpr                     { $$ = newnode(Not, NULL);   addchild($$, $2); }
    | IDENTIFIER                            { $$ = newnode(Identifier, $1); }
    | IDENTIFIER DOTLENGTH                  { $$ = newnode(Length, NULL); addchild($$, newnode(Identifier, $1)); }
    | NATURAL                               { $$ = newnode(Natural, $1); }
    | DECIMAL                               { $$ = newnode(Decimal, $1); }
    | BOOLLIT                               { $$ = newnode(BoolLit, $1); }
    | MethodInvocation                      { $$ = $1; }
    | ParseArgs                             { $$ = $1; }
    | LPAR Expr RPAR               { $$ = $2; }
    | LPAR error RPAR                       { $$ = NULL; }
    ;

Type: BOOL          { $$ = newnode(Bool, NULL); }
    | INT           { $$ = newnode(Int, NULL); }
    | DOUBLE        { $$ = newnode(Double, NULL); }
    ;

%%

/* START subroutines section */


// all needed functions are collected in the .l and ast.* files
