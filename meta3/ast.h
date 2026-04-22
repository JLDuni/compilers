#ifndef _AST_H
#define _AST_H
#include <string.h>

typedef enum {
    Program,            

    FieldDecl,          
    VarDecl,            
    MethodDecl,         
    MethodHeader,       
    MethodParams,       
    ParamDecl,          
    MethodBody,         

    Block,              
    If,                 
    While,              
    Return,             
    Print,              

    Assign,             
    Or,                 
    And,                
    Eq,                 
    Ne,                 
    Lt,                 
    Gt,                 
    Le,                 
    Ge,                 
    Add,                
    Sub,                
    Mul,                
    Div,                
    Mod,                
    Lshift,             
    Rshift,             
    Xor,                
    Not,                
    Minus,              
    Plus,               
    Length,             
    Call,               
    ParseArgs,          

    Bool,               
    BoolLit,            
    Double,             
    Decimal,            
    Identifier,         
    Int,                
    Natural,            
    StrLit,             
    StringArray,        
    Void                
} category;

enum type {
  no_type,
  integer_type,
  double_type,
  boolean_type,
  string_array_type,
  void_type,
  undef_type
};

struct node {
  category category;
  char *token;
  enum type type;
  int line;
  int column;
  struct symbol_list *local_symbols;
  struct node_list *children;
};

struct node_list {
  struct node *node;
  struct node_list *next;
};

struct node *newnode(category category, char *token, int line, int column);
void addchild(struct node *parent, struct node *child);
void show(struct node *node, int depth);
struct node_list *newlist(struct node *n);
struct node_list *append(struct node_list *list, struct node *n);
void addchildren(struct node *parent, struct node_list *list);
struct node *getchild(struct node *parent, int position);
struct node *copy_node(struct node *n);
int count_list(struct node_list *list);

const char *get_node_string(category cat);

struct node_list *copy_list(struct node_list *list);

void free_ast(struct node *n);

#endif
