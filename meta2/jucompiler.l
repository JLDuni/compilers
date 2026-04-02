%{
  #include <string.h>
  #include "y.tab.h"  
  #include "ast.h"

  int lex_only = 0;
  int show_ast = 0;
  int collumn = 1;
  int line = 1;
  char string_buffer[1024];
  bool error = false;
  bool print_tokens = false;
  bool print_error = false;

  int start_comment_line = 0;
  int start_comment_column = 0;

  int start_string_line = 0;
  int start_string_column = 0;

int yyparse(void);
void yyerror(char *);

int lex_line = 1, lex_column = 1;
int syn_line = -1, syn_column = -1;
int errors = 0;

extern struct node *ast;

  #define OUT(fmt) do { \
    if(print_tokens) printf(#fmt"\n"); \
    return fmt; \
  } while(0);

  #define OUT_VALUE(fmt, value) do { \
    if(print_tokens) printf(#fmt "(%s)\n", value); \
    yylval.lexeme = strdup(value); \
    return fmt; \
  } while(0); 

  #define OUT_STR_VALUE(fmt, value) do { \
    if(print_tokens) printf(#fmt "(\"%s\")", value); \
    yylval.lexeme = strdup(value); \
    return fmt; \
  } while(0); 

  #define PRINT_ERROR(fmt, ...) if(print_tokens || print_error) printf(fmt , ##__VA_ARGS__)
  #define PASS_TOKEN() yylval.lexeme = strdup(yytext);

  # define YY_USER_ACTION \
    collumn += yyleng;

%}
%X COMMENT
%X STRING_STATE
%X SL_COMMENT
digit   ([0-9]([0-9_]*[0-9])?)
expoent ([eE][+-]?{digit})
natural 0|[1-9]([0-9_]*[0-9])?
identifier  [a-zA-Z_$][$a-zA-Z0-9_]*
decimal {digit}*\.{digit}?{expoent}?
assign "="
lpar  "("
rpar  ")"
star  "*"
divide  "/"
plus  "+"
minus "-"
comma ","
equal "=="
ge  ">="
gt  ">"
le "<="
lbrace "{"
rbrace "}"
lsq "["
rsq "]"
lt "<"
mod "%"
ne "!="
not "!"
or "||"
and "&&"
semicollon ";"
arrow "->"
lshift "<<"
rshift ">>"
xor "^"
bool "boolean"
class "class"
print "System.out.print"
parseint "Integer.parseInt"
public "public"
return "return"
reserved "++"|"--"|"null"|"Integer"|"System"|"abstract"|"assert"|"break"|"byte"|"case"|"catch"|"char"|"const"|"continue"|"default"|"do"|"enum"|"extends"|"final"|"finally"|"float"|"for"|"goto"|"implements"|"import"|"instanceof"|"interface"|"long"|"native"|"new"|"package"|"private"|"protected"|"short"|"strictfp"|"super"|"switch"|"synchronized"|"this"|"throw"|"throws"|"transient"|"try"|"volatile"
boolit "true|false"

%%
\n                      { line++; collumn = 1; }
\r                      {}
[ \t\f]+                   {}

"//"                    { BEGIN(SL_COMMENT); }
<SL_COMMENT>.           {}
<SL_COMMENT>\n          { line++; collumn = 1; BEGIN(0); }


"/*"                    { start_comment_column = collumn - yyleng; start_comment_line = line; BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(0); }
<COMMENT>\n             { line++; collumn = 1; }
<COMMENT>.              {}
<COMMENT><<EOF>>        { PRINT_ERROR("Line %d, col %d: unterminated comment\n", start_comment_line, start_comment_column); yyterminate(); }

\"                      { BEGIN(STRING_STATE); string_buffer[0] = '\0';  start_string_line = line; start_string_column = collumn - yyleng; error = false; }
<STRING_STATE>\"              { BEGIN(0); if(!error) OUT_STR_VALUE(STRLIT, string_buffer); }
<STRING_STATE>\\([fnrt\\"])   { strcat(string_buffer, yytext); }
<STRING_STATE>[^\\\n\"]+      { strcat(string_buffer, yytext); }
<STRING_STATE>\\.             { error = true; PRINT_ERROR("Line %d, col %d: invalid escape sequence (%s)\n", line, collumn - yyleng, yytext); }
<STRING_STATE>\\             { error = true; PRINT_ERROR("Line %d, col %d: invalid escape sequence (%s)\n", line, collumn - yyleng, yytext); }
<STRING_STATE>(\r\n|\n)         { PRINT_ERROR("Line %d, col %d: unterminated string literal\n", start_string_line, start_string_column); BEGIN(0); line++; collumn = 1; }

{reserved}              { OUT_VALUE(RESERVED, yytext); }
{assign}                 {  OUT(ASSIGN); }
{lpar}                  {  OUT(LPAR); }
{rpar}                  {  OUT(RPAR); }
{star}                  {  OUT(STAR); }
{divide}                {  OUT(DIVIDE); }
{plus}                  {  OUT(PLUS); }
{minus}                 {  OUT(MINUS); }
{comma}                 { OUT(COMMA); }
{equal}                 { OUT(EQ); }
{ge}                    { OUT(GE); }
{gt}                    { OUT(GT); }
{lbrace}                { OUT(LBRACE); }
{rbrace}                { OUT(RBRACE); }
{le}                    { OUT(LE); }
{lsq}                    { OUT(LSQ); }
{rsq}                    { OUT(RSQ); }
{lt}                    { OUT(LT); }
{mod}                    { OUT(MOD); }
{ne}                    { OUT(NE); }
{not}                   { OUT(NOT); }
{or}                   { OUT(OR); }
{and}                   { OUT(AND); }
{semicollon}                   { OUT(SEMICOLON); }
{arrow}                   { OUT(ARROW); }
{rshift}                   { OUT(RSHIFT); }
{lshift}                   { OUT(LSHIFT); }
{xor}                   { OUT(XOR); }
{class}                   { OUT(CLASS); }
".length"                 { OUT(DOTLENGTH); }
{print}                   { OUT(PRINT); }
{parseint}                   { OUT(PARSEINT); }
{public}                   { OUT(PUBLIC); }
{boolit}                    { OUT_VALUE(BOOLLIT, yytext); }


"int"               {  OUT(INT); }
{bool}                   { OUT(BOOL); }
"double"                {  OUT(DOUBLE); }
"if"                    {  OUT(IF); }
"then"                  {  OUT(THEN); }
"else"                  {  OUT(ELSE); }
{return}                { OUT(RETURN); }
"static"                { OUT(STATIC); }
"String"                { OUT(STRING); }
"void"                  { OUT(VOID); }
"while"                 { OUT(WHILE); }

{natural}                {  OUT_VALUE(NATURAL, yytext); }
{decimal}                {  OUT_VALUE(DECIMAL, yytext); }
{identifier}            {  OUT_VALUE(IDENTIFIER, yytext); }
.                       { PRINT_ERROR("Line %d, col %d: illegal character (%s)\n",line, collumn - yyleng, yytext); } 
%%
extern int yylex();
int main(int argc, char *argv[]) {
  if (argc > 1) {
    if (strcmp(argv[1], "-l") == 0) {
      print_tokens = 1;
      lex_only = 1;
    } else if (strcmp(argv[1], "-e1") == 0) {
        lex_only = 1;
        print_tokens = 0;
        print_error = 1;
    } else if (strcmp(argv[1], "-t") == 0) {
        show_ast = 1;
    } else if (strcmp(argv[1], "-e2") == 0) {
        show_ast = 0; 
      }
    }

    if (lex_only) {
      while(yylex() != 0);
    } else {
        if (yyparse() == 0 && show_ast) {
            show(ast, 0); 
      }
    }
  return 0;
}
int yywrap() {  /* called on EOF, return 1 to terminate */
    return 1;
}
