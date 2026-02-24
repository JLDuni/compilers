%{
  int collumn = 1;
  int line = 1;
  char string_buffer[1024];
  bool error = false;
  bool print_tokens = false;
  # define YY_USER_ACTION \
    collumn += yyleng;
%}
%X COMMENT
%X STRING
%X SL_COMMENT
digit   [0-9]
identifier  [a-zA-Z_][a-zA-Z0-9_]*
decimal  [0-9]*\.[0-9]+
string    \"[a-zA-z]*[\f\n\r]
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
reserved "++"|"--"|"null"|"Integer"|"System"

%%
\n                      { line++; collumn = 1; }
[ \t]                   {}

"//"                    { BEGIN(SL_COMMENT); }
<SL_COMMENT>\n          { BEGIN(0); }
<SL_COMMENT>.           {}

"/*"                    { BEGIN(COMMENT); }
<COMMENT>.              {}
<COMMENT>"\n"           { line++; collumn = 1; }
<COMMENT>"*/"           { BEGIN(0); }
<COMMENT><<EOF>>        { printf("Line %d, col %d: unterminated comment\n ", line, collumn); yyterminate(); }

\"                      { BEGIN(STRING); string_buffer[0] = '\0'; }
<STRING>\"              { BEGIN(0); if(!error) printf("STRLIT(%s)\n", string_buffer); error = false;}
<STRING>\\([fnrt\\"])   { strcat(string_buffer, yytext); }
<STRING>[^\\\n\"]+      { strcat(string_buffer, yytext); }
<STRING>\\.             { error = true; printf("Line %d, col %d: invalid escape sequence (%s)\n", line, collumn, yytext); }
<STRING>\n         { printf("Line %d, col %d: unterminated string literal\n", line, collumn); BEGIN(0); }

{reserved}              { printf("RESERVED\n"); }
{assign}                 {  printf("ASSIGN\n"); }
{lpar}                  {  printf("LPAR\n"); }
{rpar}                  {  printf("RPAR\n"); }
{star}                  {  printf("STAR\n"); }
{divide}                {  printf("DIVIDE\n"); }
{plus}                  {  printf("PLUS\n"); }
{minus}                 {  printf("MINUS\n"); }
{comma}                 { printf("COMMA\n"); }
{equal}                 { printf("EQ\n"); }
{ge}                    { printf("GE\n"); }
{gt}                    { printf("GT\n"); }
{lbrace}                { printf("LBRACE\n"); }
{rbrace}                { printf("RBRACE\n"); }
{le}                    { printf("LE\n"); }
{lsq}                    { printf("LSQ\n"); }
{rsq}                    { printf("RSQ\n"); }
{lt}                    { printf("LT\n"); }
{mod}                    { printf("MOD\n"); }
{ne}                    { printf("NE\n"); }
{not}                   { printf("NOT\n"); }
{or}                   { printf("OR\n"); }
{semicollon}                   { printf("SEMICOLLON\n"); }
{arrow}                   { printf("ARROW\n"); }
{rshift}                   { printf("RSHIFT\n"); }
{lshift}                   { printf("LSHIFT\n"); }
{xor}                   { printf("XOR\n"); }
{class}                   { printf("CLASS\n"); }
".length"                 { printf("DOTLENGTH\n"); }
{print}                   { printf("PRINT\n"); }
{parseint}                   { printf("PARSEINT\n"); }
{public}                   { printf("PUBLIC\n"); }


"int"               {  printf("INT\n"); }
{bool}                   { printf("BOOL\n"); }
"double"                {  printf("DOUBLE\n"); }
"if"                    {  printf("IF\n"); }
"then"                  {  printf("THEN\n"); }
"else"                  {  printf("ELSE\n"); }
{return}                { printf("RETURN\n"); }
"static"                { printf("STATIC\n"); }
"String"                { printf("STRING\n"); }
"void"                  { printf("VOID\n"); }
"while"                 { printf("WHILE\n"); }

{digit}+                {  printf("DIGIT(%s)\n", yytext); }
{decimal}                {  printf("DECIMAL(%s)\n", yytext); }
{identifier}            {  printf("IDENTIFIER(%s)\n", yytext); }
.                       { printf("Line %d, col %d: illegal character (%s)\n",line, collumn, yytext); }
%%
extern int yylex();
int main(int argc, char *argv[]) {
    // if (strcmp(argv[1], "-l") == 0) print_tokens = true;
    yylex();    /* run the lexical analysis automaton */
    return 0;
}
int yywrap() {  /* called on EOF, return 1 to terminate */
    return 1;
}
