%{
#include <string.h>
#include "analizador.tab.h"
%}

%%

"MAIN"                  { return MAIN; }
"DEC"                   { return DEC; }
"READ"                  { return READ; }
"WRITE"                 { return WRITE; }
"IF"                    { return IF; }
"ELSE"                  { return ELSE; }
"WHILE"                 { return WHILE; }
"FOR"                   { return FOR; }

"#!"([^!\n]|"!"[^#\n])*"!#"    ;

"+"                     { return SUMA; }
"-"                     { return RESTA; }
"*"                     { return MULTIPLICACION; }
"/"                     { return DIVISION; }
":"                     { return POTENCIA; }

"=>"                    { return ASIGNACION1; }

">="                    { return MAYOR_IGUAL; }
"<="                    { return MENOR_IGUAL; }
"=="                    { return IGUAL; }
"<>"                    { return DIFERENTE; }
"<"                     { return MENOR; }
">"                     { return MAYOR; }

"&&"                    { return AND; }
"||"                    { return OR; }

"("                     { return PAR_IZQ; }
")"                     { return PAR_DER; }
"{"                     { return LLAVE_IZQ; }
"}"                     { return LLAVE_DER; }

";"                     { return PUNTOYCOMA; }
","                     { return COMA; }

\$[a-zA-Z][a-zA-Z0-9]*  { strcpy(yylval.cadena,yytext); return VARIABLE; }
[0-9]+                  { strcpy(yylval.cadena,yytext); return NUMERO; }

\n                      { yylineno++; }
[ \t]+                  ;
.                       { return yytext[0]; }

%%

int yywrap()
{
    return 1;
}