## Author

João Loreto Dias 2023214314

## Introduction

This projects objective was to implement a compiler for a sublanguage of Java, Juc. In this report we will go through the grammar, algorithms and data structures chosen required to complete this project. I completed the lexical analyzer, Syntax analyzer, Semantic Analyzer and code generation with a llvm that is a Intermidiate Representation for Assembly. Lex was used for tokenization but it is not asked to go in detail for that.

## Rewritten Grammar

For this project a Abstract Syntax Tree was required to represent the syntax of the language. Lets go from top to bottom.

Program: CLASS IDENTIFIER LBRACE declarations RBRACE 

This will always be the root node of any Juc program, which will be a Class.

declarations:     
            | declarations MethodDecl
            | declarations FieldDecl 
            | declarations SEMICOLON 

These are all the possible declaration we may have inside a Class. Nothing, Method Declaration, Field Declaration or even just a semicolon. Having "declaration" stated in each production will allow us to have multiple declarations for our program, this strategy will be used almost always.

FieldDecl: PUBLIC STATIC Type IDENTIFIER MultipleIdentifiers SEMICOLON
         | error SEMICOLON

Lets start by explaining every declaration first. This will be a Field Declaration that may look like this: 

public static int var, var2;

If it does not have the format depicted on the grammar it will print the syntax error.

MethodDecl: PUBLIC STATIC MethodHeader MethodBody

A Method Declaration will be represented by a Method Header and a Method Body.

MethodHeader: Type IDENTIFIER LPAR OptionalFormalParams RPAR
            | VOID IDENTIFIER LPAR OptionalFormalParams RPAR

A Method Header is the part of the defenition containing the Method parameters, return type and identifier. Example:

int func(int num1, double num2)

The void type is seperate of type production to avoid things like a Field declaration being declared with type void and give the according error.

public static void var;

MethodBody: LBRACE BodyContent RBRACE

This will be everything in Method Body.


