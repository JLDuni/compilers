%{
  int collumn = 1;
  int line = 1;
  char string_buffer[1024];
  bool error = false;
  bool print_tokens = false;
  bool print_error = false;
  # define YY_USER_ACTION \
    collumn += yyleng;

  #define OUT(fmt, ...) if(print_tokens) printf(fmt , ##__VA_ARGS__)
  #define PRINT_ERROR(fmt, ...) if(print_tokens || print_error) printf(fmt , ##__VA_ARGS__)
%}
%X COMMENT
%X STRING
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

%%
\n                      { line++; collumn = 1; }
\r                      {}
[ \t]+                   {}

"//"                    { BEGIN(SL_COMMENT); }
<SL_COMMENT>.           {}
<SL_COMMENT>\n          { line++; BEGIN(0); }


"/*"                    { BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(0); }
<COMMENT>\n             { line++; collumn = 1; }
<COMMENT>.              {}
<COMMENT><<EOF>>        { PRINT_ERROR("Line %d, col %d: unterminated comment\n ", line, collumn); yyterminate(); }

\"                      { BEGIN(STRING); string_buffer[0] = '\0'; }
<STRING>\"              { BEGIN(0); if(!error) OUT("STRLIT(%s)\n", string_buffer); error = false;}
<STRING>\\([fnrt\\"])   { strcat(string_buffer, yytext); }
<STRING>[^\\\n\"]+      { strcat(string_buffer, yytext); }
<STRING>\\.             { error = true; PRINT_ERROR("Line %d, col %d: invalid escape sequence (%s)\n", line, collumn, yytext); }
<STRING>\n         { PRINT_ERROR("Line %d, col %d: unterminated string literal\n", line, collumn); BEGIN(0); line++; }

{reserved}              { OUT("RESERVED(%s)\n", yytext); }
{assign}                 {  OUT("ASSIGN\n"); }
{lpar}                  {  OUT("LPAR\n"); }
{rpar}                  {  OUT("RPAR\n"); }
{star}                  {  OUT("STAR\n"); }
{divide}                {  OUT("DIVIDE\n"); }
{plus}                  {  OUT("PLUS\n"); }
{minus}                 {  OUT("MINUS\n"); }
{comma}                 { OUT("COMMA\n"); }
{equal}                 { OUT("EQ\n"); }
{ge}                    { OUT("GE\n"); }
{gt}                    { OUT("GT\n"); }
{lbrace}                { OUT("LBRACE\n"); }
{rbrace}                { OUT("RBRACE\n"); }
{le}                    { OUT("LE\n"); }
{lsq}                    { OUT("LSQ\n"); }
{rsq}                    { OUT("RSQ\n"); }
{lt}                    { OUT("LT\n"); }
{mod}                    { OUT("MOD\n"); }
{ne}                    { OUT("NE\n"); }
{not}                   { OUT("NOT\n"); }
{or}                   { OUT("OR\n"); }
{and}                   { OUT("AND\n"); }
{semicollon}                   { OUT("SEMICOLLON\n"); }
{arrow}                   { OUT("ARROW\n"); }
{rshift}                   { OUT("RSHIFT\n"); }
{lshift}                   { OUT("LSHIFT\n"); }
{xor}                   { OUT("XOR\n"); }
{class}                   { OUT("CLASS\n"); }
".length"                 { OUT("DOTLENGTH\n"); }
{print}                   { OUT("PRINT\n"); }
{parseint}                   { OUT("PARSEINT\n"); }
{public}                   { OUT("PUBLIC\n"); }


"int"               {  OUT("INT\n"); }
{bool}                   { OUT("BOOL\n"); }
"double"                {  OUT("DOUBLE\n"); }
"if"                    {  OUT("IF\n"); }
"then"                  {  OUT("THEN\n"); }
"else"                  {  OUT("ELSE\n"); }
{return}                { OUT("RETURN\n"); }
"static"                { OUT("STATIC\n"); }
"String"                { OUT("STRING\n"); }
"void"                  { OUT("VOID\n"); }
"while"                 { OUT("WHILE\n"); }

{natural}+                {  OUT("NATURAL(%s)\n", yytext); }
{decimal}                {  OUT("DECIMAL(%s)\n", yytext); }
{identifier}            {  OUT("IDENTIFIER(%s)\n", yytext); }
.                       { PRINT_ERROR("Line %d, col %d: illegal character (%s)\n",line, collumn, yytext); }
%%
extern int yylex();
int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "-l") == 0){
      print_tokens = true;
    } else if(argc >= 1 || strcmp(argv[1], "-e1") == 0) {
      print_error = true;
    }
    yylex();    /* run the lexical analysis automaton */
    return 0;
}
int yywrap() {  /* called on EOF, return 1 to terminate */
    return 1;
}
