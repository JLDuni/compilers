%option 8bit
%{
  int collumn = 1;
  int line = 1;

  bool error = false;
  bool print_tokens = false;
  bool print_error = false;

  int start_comment_line = 0;
  int start_comment_column = 0;

  int start_string_line = 0;
  int start_string_column = 0;

  char *string_buffer = NULL;
  int string_len = 0;

  void append_text(char *text, int len) {
    string_buffer = realloc(string_buffer, string_len + len);
    memcpy(string_buffer + string_len, text, len);
    string_len += len;
    string_buffer[string_len] = '\0';
  }

  #define OUT(fmt, ...) if(print_tokens) printf(fmt , ##__VA_ARGS__)
  #define PRINT_ERROR(fmt, ...) if(print_tokens || print_error) printf(fmt , ##__VA_ARGS__)

  # define YY_USER_ACTION \
    collumn += yyleng;

%}
%X COMMENT
%X STRING
%X SL_COMMENT
digit   ([0-9]([0-9_]*[0-9])?)
expoent ([eE][+-]?{digit})
natural 0|[1-9]([0-9_]*[0-9])?
identifier  [a-zA-Z_$][$a-zA-Z0-9_]*
decimal ({digit}\.{digit}?{expoent}?)|(\.{digit}{expoent}?)|({digit}{expoent})
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
boollit "true"|"false"

%%
(\r|\r\n|\n)      { line++; collumn = 1; }
[ \t\f]+                   {}

"//"                    { BEGIN(SL_COMMENT); }
<SL_COMMENT>[^\n\r]*           {}
<SL_COMMENT>(\r|\n|\r\n)          { line++; collumn = 1; BEGIN(0); }


"/*"                    { start_comment_column = collumn - yyleng; start_comment_line = line; BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(0); }
<COMMENT>(\r\n|\n|\r)             { line++; collumn = 1; }
<COMMENT><<EOF>>        { PRINT_ERROR("Line %d, col %d: unterminated comment\n", start_comment_line, start_comment_column); yyterminate(); }
<COMMENT>.              {}

\"                      { BEGIN(STRING); start_string_line = line; start_string_column = collumn - yyleng; error = false;
                        string_buffer = malloc(1); string_len = 0; string_buffer[0] = '\0'; }
<STRING>\"              { BEGIN(0); if(!error) OUT("STRLIT(\"%s\")\n", string_buffer); }
<STRING>\\([fnrt\\"])   { append_text(yytext, yyleng); }
<STRING>\\[^\n\r]|\\    { error = true; PRINT_ERROR("Line %d, col %d: invalid escape sequence (%s)\n", line, collumn - yyleng, yytext); }
<STRING>(\r\n|\n|\r)         { PRINT_ERROR("Line %d, col %d: unterminated string literal\n", start_string_line, start_string_column); BEGIN(0); line++; collumn = 1; }
<STRING><<EOF>>         { PRINT_ERROR("Line %d, col %d: unterminated string literal\n", start_string_line, start_string_column); yyterminate(); }
<STRING>.      { append_text(yytext, yyleng); }

{parseint}                   { OUT("PARSEINT\n"); }
{print}                   { OUT("PRINT\n"); }
".length"                 { OUT("DOTLENGTH\n"); }

{lshift}                   { OUT("LSHIFT\n"); }
{rshift}                   { OUT("RSHIFT\n"); }
{arrow}                   { OUT("ARROW\n"); }
{or}                   { OUT("OR\n"); }
{and}                   { OUT("AND\n"); }
{ne}                    { OUT("NE\n"); }
{le}                    { OUT("LE\n"); }
{ge}                    { OUT("GE\n"); }
{equal}                 { OUT("EQ\n"); }

{class}                   { OUT("CLASS\n"); }
"int"               {  OUT("INT\n"); }
{bool}                   { OUT("BOOL\n"); }
"double"                {  OUT("DOUBLE\n"); }
"if"                    {  OUT("IF\n"); }
"else"                  {  OUT("ELSE\n"); }
{return}                { OUT("RETURN\n"); }
"static"                { OUT("STATIC\n"); }
"String"                { OUT("STRING\n"); }
"void"                  { OUT("VOID\n"); }
"while"                 { OUT("WHILE\n"); }
{public}                   { OUT("PUBLIC\n"); }
{boollit}               { OUT("BOOLLIT(%s)\n", yytext); }

{reserved}              { OUT("RESERVED(%s)\n", yytext); }

{decimal}                {  OUT("DECIMAL(%s)\n", yytext); }
{natural}                {  OUT("NATURAL(%s)\n", yytext); }
{identifier}            {  OUT("IDENTIFIER(%s)\n", yytext); }

{assign}                 {  OUT("ASSIGN\n"); }
{lpar}                  {  OUT("LPAR\n"); }
{rpar}                  {  OUT("RPAR\n"); }
{star}                  {  OUT("STAR\n"); }
{divide}                {  OUT("DIV\n"); }
{plus}                  {  OUT("PLUS\n"); }
{minus}                 {  OUT("MINUS\n"); }
{comma}                 { OUT("COMMA\n"); }
{gt}                    { OUT("GT\n"); }
{lbrace}                { OUT("LBRACE\n"); }
{rbrace}                { OUT("RBRACE\n"); }
{lsq}                    { OUT("LSQ\n"); }
{rsq}                    { OUT("RSQ\n"); }
{lt}                    { OUT("LT\n"); }
{mod}                    { OUT("MOD\n"); }
{not}                   { OUT("NOT\n"); }
{semicollon}                   { OUT("SEMICOLON\n"); }
{xor}                   { OUT("XOR\n"); }

.                       { PRINT_ERROR("Line %d, col %d: illegal character (%s)\n",line, collumn - yyleng, yytext); } 
%%
extern int yylex();
int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "-l") == 0){
      print_tokens = true;
    } else if(argc >= 1 || strcmp(argv[1], "-e1") == 0) {
      print_error = true;
    }
    yylex();    /* run the lexical analysis automaton */
    free(string_buffer);
    return 0;
}
int yywrap() {  /* called on EOF, return 1 to terminate */
    return 1;
}
