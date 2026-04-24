/* START definitions section -- C code delimited by %{ ... %} and token declarations */

%{

#include <stdio.h>
#include "ast.h"

int yylex(void);
void yyerror(char *);

struct node *ast;

#define CREATE_NODE(category, token, loc) newnode(category, token, loc.first_line, loc.first_column)
%}

%union{
    char *lexeme;
    struct node_list *node_list;
    struct node *node;
}


%locations
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

Program: CLASS IDENTIFIER LBRACE declarations RBRACE                { ast = $$ = CREATE_NODE(Program, NULL, @$);
                                                                     struct node *identifier_node = CREATE_NODE(Identifier, $2, @2); 
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
            ;

FieldDecl: PUBLIC STATIC Type IDENTIFIER MultipleIdentifiers SEMICOLON      { struct node *decl = CREATE_NODE(FieldDecl, NULL, @$); 
                                                                              addchild(decl, $3); 
                                                                              addchild(decl, CREATE_NODE(Identifier, $4, @4)); 

                                                                              struct node_list *list = newlist(decl); 
                                                                              
                                                                              struct node_list *curr = $5;
                                                                              while(curr != NULL) {
                                                                                struct node *current_decl = CREATE_NODE(FieldDecl, NULL, @$);
                                                                                addchild(current_decl, copy_node($3));
                                                                                addchild(current_decl, curr->node);
                                                                                list = append(list, current_decl);
                                                                                curr = curr->next;
                                                                              } 
                                                                              $$ = list; 
                                                                            }
         | error SEMICOLON                                            {$$ = NULL; }
         ;

MethodDecl: PUBLIC STATIC MethodHeader MethodBody     { $$ = CREATE_NODE(MethodDecl, NULL, @$); 
                                                      struct node_list *list = newlist($3); 
                                                      append(list, $4);
                                                      addchildren($$, list); }
;

MethodHeader: Type IDENTIFIER LPAR OptionalFormalParams RPAR { $$ = CREATE_NODE(MethodHeader, NULL, @$); 
                                                      struct node_list *list = newlist($1);
                                                      append(list, CREATE_NODE(Identifier, $2, @2)); 
                                                      list = append(list, $4); 
                                                      addchildren($$, list); }

            | VOID IDENTIFIER LPAR OptionalFormalParams RPAR { $$ = CREATE_NODE(MethodHeader, NULL, @$); 
                                                               struct node_list *list = newlist(CREATE_NODE(Void, NULL, @1)); 
                                                               append(list, CREATE_NODE(Identifier, $2, @2)); 
                                                               append(list, $4); 
                                                               addchildren($$, list); }

            
            ;

MethodBody: LBRACE BodyContent RBRACE     { $$ = CREATE_NODE(MethodBody, NULL, @$); 
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

VarDecl: Type IDENTIFIER MultipleIdentifiers SEMICOLON    { struct node *decl = CREATE_NODE(VarDecl, NULL, @$); 
                                                      addchild(decl, $1); 
                                                      addchild(decl, CREATE_NODE(Identifier, $2, @2)); 
                                                      struct node_list *list = newlist(decl); 
                                                      
                                                      struct node_list *curr = $3;
                                                      while(curr != NULL) {
                                                        struct node *current_decl = CREATE_NODE(VarDecl, NULL, @$);
                                                        addchild(current_decl, copy_node($1));
                                                        addchild(current_decl, curr->node);
                                                        list = append(list, current_decl);
                                                        curr = curr->next;
                                                      } 
                                                      $$ = list;
                                                      } 
        ;

MultipleIdentifiers: { $$ = NULL; }
                   | MultipleIdentifiers COMMA IDENTIFIER   { $$ = append($1, CREATE_NODE(Identifier, $3, @3)); }
                   ;

Statement: LBRACE StatementList RBRACE      { int count = count_list($2); 
                                              if (count > 1) {$$ = CREATE_NODE(Block, NULL, @$); addchildren($$, $2);}
                                              else if (count == 1) $$ = $2->node;
                                              else{ $$ = NULL; } }
        ;

Statement: IF LPAR Expr RPAR Statement %prec IF_PREC         { $$ = CREATE_NODE(If, NULL, @$); 
                                                    addchild($$, $3); 
                                                    addchild($$, $5 != NULL ? $5 : CREATE_NODE(Block, NULL, @$)); 
                                                    addchild($$, CREATE_NODE(Block, NULL, @$)); }
         | IF LPAR Expr RPAR Statement ELSE Statement     { $$ = CREATE_NODE(If, NULL, @$); 
                                                    addchild($$, $3); 
                                                    addchild($$, $5 != NULL ? $5 : CREATE_NODE(Block, NULL, @$)); 
                                                    addchild($$, $7 != NULL ? $7 : CREATE_NODE(Block, NULL, @$)); }

         | WHILE LPAR Expr RPAR Statement                 { $$ = CREATE_NODE(While, NULL, @$); 
                                                    addchild($$, $3); 
                                                    addchild($$, $5 != NULL ? $5 : CREATE_NODE(Block, NULL, @$)); }

         | RETURN SEMICOLON                               { $$ = CREATE_NODE(Return, NULL, @$); }
         | RETURN Expr SEMICOLON                         { $$ = CREATE_NODE(Return, NULL, @$); 
                                                    addchild($$, $2);}

        | MethodInvocation SEMICOLON                     { $$ =  $1; }
        | Assignment SEMICOLON                     { $$ =  $1; }
        | ParseArgs SEMICOLON                     { $$ =  $1; }
        | PRINT LPAR Expr RPAR SEMICOLON           { $$ = CREATE_NODE(Print, NULL, @$); 
                                                      addchild($$, $3); }

        | PRINT LPAR STRLIT RPAR SEMICOLON           { $$ = CREATE_NODE(Print, NULL, @$); 
                                                      addchild($$, CREATE_NODE(StrLit, $3, @3)); }
        | error SEMICOLON                             { $$ = NULL; }
        | SEMICOLON                                   { $$ = NULL; }
        ;

StatementList:          { $$ = NULL; }
             | StatementList Statement    { if( $2 != NULL) $$ = append($1, $2); 
                                            else $$ = $1; }
             ;

MethodInvocation: IDENTIFIER LPAR ArgsList RPAR     { $$ = CREATE_NODE(Call, NULL, @$); 
                                                      struct node *id_node = CREATE_NODE(Identifier, $1, @1); 
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

OptionalFormalParams:                 { $$ = CREATE_NODE(MethodParams, NULL, @$); }
                    | FormalParams    { $$ = CREATE_NODE(MethodParams, NULL, @$); 
                                      addchildren($$, $1); }
                    ;
         
FormalParams: TypeList                              { $$ = $1; }             
            | STRING LSQ RSQ IDENTIFIER            { struct node *param_declaration = CREATE_NODE(ParamDecl, NULL, @$); 
                                                     struct node_list *children = newlist(CREATE_NODE(StringArray, NULL, @1)); 
                                                     append(children, CREATE_NODE(Identifier, $4, @4)); 
                                                     addchildren(param_declaration, children); 
                                                     $$ = newlist(param_declaration); }

            ;

TypeList: Type IDENTIFIER { struct node *param_declaration = CREATE_NODE(ParamDecl, NULL, @$); 
                                struct node_list *children = newlist($1); 
                                append(children, CREATE_NODE(Identifier, $2, @2));
                                addchildren(param_declaration, children); 
                                $$ = newlist(param_declaration);
                                }
            | TypeList COMMA Type IDENTIFIER   { struct node *param_declaration = CREATE_NODE(ParamDecl, NULL, @$); 
                                                     struct node_list *children = newlist($3); 
                                                     append(children, CREATE_NODE(Identifier, $4, @4)); 
                                                     addchildren(param_declaration, children); 
                                                     $$ = append($1, param_declaration); }

            ;

Assignment: IDENTIFIER ASSIGN Expr               { $$ = CREATE_NODE(Assign, NULL, @2); 
                                                    struct node_list *list = newlist(CREATE_NODE(Identifier, $1, @1)); 
                                                    append(list, $3); 
                                                    addchildren($$, list); }
           ;
            

ParseArgs: PARSEINT LPAR IDENTIFIER LSQ Expr RSQ RPAR     { $$ = CREATE_NODE(ParseArgs, NULL, @$); 
                                                          struct node_list *list = newlist(CREATE_NODE(Identifier, $3, @3)); 
                                                          append(list, $5); 
                                                          addchildren($$, list); }

         | PARSEINT LPAR error RPAR                       { $$ = NULL; }
         ;

Expr: Assignment        { $$ = $1; }
    | OperationExpr     { $$ = $1; }

OperationExpr: OperationExpr PLUS OperationExpr    { $$ = CREATE_NODE(Add, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr MINUS OperationExpr   { $$ = CREATE_NODE(Sub, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr STAR OperationExpr    { $$ = CREATE_NODE(Mul, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr DIV OperationExpr     { $$ = CREATE_NODE(Div, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr MOD OperationExpr     { $$ = CREATE_NODE(Mod, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr AND OperationExpr     { $$ = CREATE_NODE(And, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr OR OperationExpr      { $$ = CREATE_NODE(Or, NULL, @2);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr XOR OperationExpr     { $$ = CREATE_NODE(Xor, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr LSHIFT OperationExpr  { $$ = CREATE_NODE(Lshift, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr RSHIFT OperationExpr  { $$ = CREATE_NODE(Rshift, NULL, @2); addchildren($$, append(newlist($1), $3)); }
    | OperationExpr EQ OperationExpr      { $$ = CREATE_NODE(Eq, NULL, @2);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr NE OperationExpr      { $$ = CREATE_NODE(Ne, NULL, @2);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr GE OperationExpr      { $$ = CREATE_NODE(Ge, NULL, @2);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr GT OperationExpr      { $$ = CREATE_NODE(Gt, NULL, @2);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr LE OperationExpr      { $$ = CREATE_NODE(Le, NULL, @2);  addchildren($$, append(newlist($1), $3)); }
    | OperationExpr LT OperationExpr      { $$ = CREATE_NODE(Lt, NULL, @2);  addchildren($$, append(newlist($1), $3)); }

    | MINUS OperationExpr %prec UNARY_MINUS { $$ = CREATE_NODE(Minus, NULL, @1); addchild($$, $2); }
    | PLUS OperationExpr %prec UNARY_PLUS   { $$ = CREATE_NODE(Plus, NULL, @1);  addchild($$, $2); }
    | NOT OperationExpr                     { $$ = CREATE_NODE(Not, NULL, @1);   addchild($$, $2); }
    | IDENTIFIER                            { $$ = CREATE_NODE(Identifier, $1, @1); }
    | IDENTIFIER DOTLENGTH                  { $$ = CREATE_NODE(Length, NULL, @2); addchild($$, CREATE_NODE(Identifier, $1, @1)); }
    | NATURAL                               { $$ = CREATE_NODE(Natural, $1, @1); }
    | DECIMAL                               { $$ = CREATE_NODE(Decimal, $1, @1); }
    | BOOLLIT                               { $$ = CREATE_NODE(BoolLit, $1, @1); }
    | MethodInvocation                      { $$ = $1; }
    | ParseArgs                             { $$ = $1; }
    | LPAR Expr RPAR               { $$ = $2; }
    | LPAR error RPAR                       { $$ = NULL; }
    ;

Type: BOOL          { $$ = CREATE_NODE(Bool, NULL, @$); }
    | INT           { $$ = CREATE_NODE(Int, NULL, @$); }
    | DOUBLE        { $$ = CREATE_NODE(Double, NULL, @$); }
    ;

%%

/* START subroutines section */


// all needed functions are collected in the .l and ast.* files
