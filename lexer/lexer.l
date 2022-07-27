%{
#include <cctype>
#include <cstdio>
#include <string>
#include <iostream>
#define YYEOF (0)
int yylex();
int main() {
  // yyout = stderr; // to be same with clang -cc1 -dump-tokens
  do {
  } while (yylex() != YYEOF);
}
// std::string yyloc = "<stdin>";
std::string path="";
int yyrow = 1, yycolumn = 1, yycolpre = 1, pre_row = 0, pre_col = 1;

#define YY_USER_ACTION                                                         \
  do {                                                                         \
    yycolumn += yyleng;                                                        \
  } while (0);
void lexPrint(std::string str){
  std::string start="";
  std::string leading="";
  if(pre_row != yyrow){
    start=" [StartOfLine]";
  }
  if(pre_col != yycolumn - yyleng){
    leading = " [LeadingSpace]";
  }
  pre_row = yyrow;
  pre_col = yycolumn;
  if(str == "eof"){
    fprintf(yyout, "%s '%s'\t\tLoc=<%s:%d:%d>\n", str.c_str(), yytext,  path.c_str(), yyrow > 1 ? yyrow - 1 : yyrow, yycolpre - yyleng);
  }else{
    fprintf(yyout, "%s '%s'\t%s%s\tLoc=<%s:%d:%d>\n", str.c_str(), yytext, start.c_str(), leading.c_str(), path.c_str(), yyrow, yycolumn - yyleng);
  }
  
}
%}
%option noyywrap
%%



#\ [0-9]+\ \".*\"(\ |.)*\n {
  std::string temp = std::string(yytext);
  
  //计算行和列
  int num = 0;
  int idx=2;
  while(temp[idx]!=' '){
    num *= 10;
    num += (int)(temp[idx] - '0');
    idx++;
  }
  yyrow = num;
  yycolpre = yycolumn;
  yycolumn = 1;

  //计算路径
  idx+=2;
  int start = idx;
  while(temp[idx]!='"'){
    idx++;
  }
  int len = idx - start;
  path = temp.substr(start, len);
}

\"[ \(\)\[\]\"\@\\n!\/]*\" {
  lexPrint("string_literal");
  return ~YYEOF;
}

int {
  lexPrint("int");
  return ~YYEOF;
}

char {
  lexPrint("char");
  return ~YYEOF;
}

return {
  lexPrint("return");
  return ~YYEOF;
}

const {
  lexPrint("const");
  return ~YYEOF;
}

break {
  lexPrint("break");
  return ~YYEOF;
}

continue {
  lexPrint("continue");
  return ~YYEOF;
}

for {
  lexPrint("for");
  return ~YYEOF;
}

void {
  lexPrint("void");
  return ~YYEOF;
}

if {
  lexPrint("if");
  return ~YYEOF;
}

else {
  lexPrint("else");
  return ~YYEOF;
}


while {
  lexPrint("while");
  return ~YYEOF;
}

do {
  lexPrint("do");
  return ~YYEOF;
}

\( {
  lexPrint("l_paren");
  return ~YYEOF;
}

\) {
  lexPrint("r_paren");
  return ~YYEOF;
}

\{ {
  lexPrint("l_brace");
  return ~YYEOF;
}

\} {
  lexPrint("r_brace");
  return ~YYEOF;
}

; {
  lexPrint("semi");
  return ~YYEOF;
}

! {
  lexPrint("exclaim");
  return ~YYEOF;
}

!= {
  lexPrint("exclaimequal");
  return ~YYEOF;
}

\<\= {
  lexPrint("lessequal");
  return ~YYEOF;
}

\>\= {
  lexPrint("greaterequal");
  return ~YYEOF;
}

== {
  lexPrint("equalequal");
  return ~YYEOF;
}

&& {
  lexPrint("ampamp");
  return ~YYEOF;
}

& {
  lexPrint("amp");
  return ~YYEOF;
}

\|\| {
  lexPrint("pipepipe");
  return ~YYEOF;
}

\| {
  lexPrint("pipe");
  return ~YYEOF;
}

= {
  lexPrint("equal");
  return ~YYEOF;
}

\+ {
  lexPrint("plus");
  return ~YYEOF;
}

\- {
  lexPrint("minus");
  return ~YYEOF;
}

\* {
  lexPrint("star");
  return ~YYEOF;
}

\/ {
  lexPrint("slash");
  return ~YYEOF;
}

\% {
  lexPrint("percent");
  return ~YYEOF;
}

, {
  lexPrint("comma");
  return ~YYEOF;
}

\[ {
  lexPrint("l_square");
  return ~YYEOF;
}

\] {
  lexPrint("r_square");
  return ~YYEOF;
}

\> {
  lexPrint("greater");
  return ~YYEOF;
}

\< {
  lexPrint("less");
  return ~YYEOF;
}

(0x[0-9a-fA-F]+)|([0-9]+) {
  lexPrint("numeric_constant");
  return ~YYEOF;
}

[a-zA-Z_][a-zA-Z_0-9]* {
  lexPrint("identifier");
  return ~YYEOF;
}

<<EOF>> {
  // fprintf(yyout, "eof ''\t\tLoc=<<stdin>:%d:%d>\n",
  //         yyrow > 1 ? yyrow - 1 : yyrow, yycolpre - yyleng);
  lexPrint("eof");
  return YYEOF;
}

#.* {
  --yyrow;
}

\n {
  ++yyrow;
  yycolpre = yycolumn;
  pre_col = 1;
  yycolumn = 1;
}

[\f\r\t\v ] {}

. {
  lexPrint("unknown");
  return ~YYEOF;
}

%%