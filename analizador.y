// definiciones o declaraciones
%{
#include <stdio.h>

void yyerror(char *mensaje);
int yylex();
extern int yylineno;
extern char *yytext;
%}

// Tokens
%token MAIN DEC READ WRITE IF ELSE WHILE FOR
%token VARIABLE NUMERO
%token SUMA RESTA MULTIPLICACION DIVISION POTENCIA
%token PAR_IZQ PAR_DER
%token LLAVE_IZQ LLAVE_DER
%token PUNTOYCOMA COMA
%token ASIGNACION1
%token MENOR MAYOR MAYOR_IGUAL MENOR_IGUAL IGUAL DIFERENTE
%token AND OR

%%

// reglas gramaticales

programa:
    MAIN bloque
    ;

bloque:
    LLAVE_IZQ lista_sentencias LLAVE_DER
    ;

lista_sentencias:
    sentencia lista_sentencias
    |
    ;

sentencia:
    sentencia_simple PUNTOYCOMA
    | if_sentencia
    | while_sentencia
    | for_sentencia
    ;

sentencia_simple:
    declaracion
    | asignacion
    | lectura
    | escritura
    ;

declaracion:
    DEC lista_variables
    ;

lista_variables:
    VARIABLE mas_variables
    ;

mas_variables:
    COMA VARIABLE mas_variables
    |
    ;

asignacion:
    VARIABLE operador_asignacion expresion
    ;

operador_asignacion:
    ASIGNACION1
    ;

lectura:
    READ VARIABLE
    ;

escritura:
    WRITE VARIABLE
    ;

// operaciones matematicas

expresion:
    expresion SUMA termino
    | expresion RESTA termino
    | termino
    ;

termino:
    termino MULTIPLICACION potencia
    | termino DIVISION potencia
    | potencia
    ;

potencia:
    factor POTENCIA potencia
    | factor
    ;

factor:
    PAR_IZQ expresion PAR_DER
    | VARIABLE
    | NUMERO
    | SUMA factor
    | RESTA factor
    ;

// condicionales

if_sentencia:
    IF PAR_IZQ condicion PAR_DER bloque sino
    ;

sino:
    ELSE bloque
    | ELSE if_sentencia
    |
    ;

// ciclos

while_sentencia:
    WHILE PAR_IZQ condicion PAR_DER bloque
    ;

for_sentencia:
    FOR PAR_IZQ asignacion PUNTOYCOMA condicion PUNTOYCOMA asignacion PAR_DER bloque
    ;

// condiciones

condicion:
    condicion_or
    ;

condicion_or:
    condicion_and condicion_or_prima
    ;

condicion_or_prima:
    OR condicion_and condicion_or_prima
    |
    ;

condicion_and:
    comparacion condicion_and_prima
    ;

condicion_and_prima:
    AND comparacion condicion_and_prima
    |
    ;

comparacion:
    expresion operador_relacional expresion
    ;

operador_relacional:
    MENOR
    | MAYOR
    | MAYOR_IGUAL
    | MENOR_IGUAL
    | IGUAL
    | DIFERENTE
    ;

%%

// funciones

void yyerror(char *mensaje)
{
    printf("Error sintactico en la linea %d: %s cerca de '%s'\n",
           yylineno, mensaje, yytext);
}

int main()
{
    if (yyparse() == 0){
        printf("Analisis sintactico correcto\n");
    }

    return 0;
}