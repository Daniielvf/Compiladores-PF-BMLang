// definiciones o declaraciones
%{
#include <stdio.h>
#include <string.h>

void yyerror(char *mensaje);
int yylex();
void nuevaTemp(char *temporal);
void nuevaEtiqueta(char *etiqueta);

%}

%union{
    char cadena[50];
}

// Tokens
%token MAIN DEC READ WRITE IF ELSE WHILE FOR
%token <cadena> VARIABLE NUMERO
%token SUMA RESTA MULTIPLICACION DIVISION POTENCIA
%token PAR_IZQ PAR_DER
%token LLAVE_IZQ LLAVE_DER
%token PUNTOYCOMA COMA
%token ASIGNACION1
%token MENOR MAYOR MAYOR_IGUAL MENOR_IGUAL IGUAL DIFERENTE
%token AND OR

%type <cadena> expresion termino potencia factor comparacion operador_relacional
%type <cadena> condicion condicion_or condicion_or_prima
%type <cadena> condicion_and condicion_and_prima

%%

// reglas gramaticales

programa:
    MAIN
    {
        printf("main:\n");
    }
    bloque
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
    {
        printf("%s=%s\n", $1, $3);
    }
    ;

operador_asignacion:
    ASIGNACION1
    ;

lectura:
    READ VARIABLE
    {
        printf("call input;\n");
        printf("pop %s;\n", $2);
    }
    ;

escritura:
    WRITE VARIABLE
    {
        printf("push %s;\n", $2);
        printf("call output;\n");
    }
    ;

// operaciones matematicas

expresion:
    expresion SUMA termino
    {
        nuevaTemp($$);
        printf("%s=%s+%s\n", $$, $1, $3);
    }
    | expresion RESTA termino
    {
        nuevaTemp($$);
        printf("%s=%s-%s\n", $$, $1, $3);
    }
    | termino
    {
        strcpy($$, $1);
    }
    ;

termino:
    termino MULTIPLICACION potencia
    {
        nuevaTemp($$);
        printf("%s=%s*%s\n", $$, $1, $3);
    }
    | termino DIVISION potencia
    {
        nuevaTemp($$);
        printf("%s=%s/%s\n", $$, $1, $3);
    }
    | potencia
    {
        strcpy($$, $1);
    }
    ;

potencia:
    factor POTENCIA potencia
    {
        nuevaTemp($$);
        printf("%s=%s:%s\n", $$, $1, $3);
    }
    | factor
    {
        strcpy($$, $1);
    }
    ;

factor:
    PAR_IZQ expresion PAR_DER
    {
        strcpy($$, $2);
    }
    | VARIABLE
    {
        strcpy($$, $1);
    }
    | NUMERO
    {
        strcpy($$, $1);
    }
    | SUMA factor
    {
        strcpy($$, $2);
    }
    | RESTA factor
    {
        nuevaTemp($$);
        printf("%s=0-%s\n", $$, $2);
    }
    ;

// condicionales

if_sentencia:
    IF PAR_IZQ condicion PAR_DER
    {
        nuevaEtiqueta($<cadena>$);
        printf("ifZ %s goto %s\n", $3, $<cadena>$);
    }
    bloque
    {
        nuevaEtiqueta($<cadena>$);
        printf("goto %s\n", $<cadena>$);
        printf("%s:\n", $<cadena>5);
    }
    sino
    {
        printf("%s:\n", $<cadena>7);
    }
    ;

sino:
    ELSE bloque
    | ELSE if_sentencia
    |
    ;

// ciclos

while_sentencia:
    WHILE
    {
        nuevaEtiqueta($<cadena>$);
        printf("%s:\n", $<cadena>$);
    }
    PAR_IZQ condicion PAR_DER
    {
        nuevaEtiqueta($<cadena>$);
        printf("ifZ %s goto %s\n", $4, $<cadena>$);
    }
    bloque
    {
        printf("goto %s\n", $<cadena>2);
        printf("%s:\n", $<cadena>6);
    }
    ;

for_sentencia:
    FOR PAR_IZQ asignacion PUNTOYCOMA
    {
        nuevaEtiqueta($<cadena>$);
        printf("%s:\n", $<cadena>$);
    }
    condicion PUNTOYCOMA
    {
        nuevaEtiqueta($<cadena>$);
        printf("ifZ %s goto %s\n", $6, $<cadena>$);
    }
    {
        nuevaEtiqueta($<cadena>$);
        printf("goto %s\n", $<cadena>$);
    }
    {
        nuevaEtiqueta($<cadena>$);
        printf("%s:\n", $<cadena>$);
    }
    asignacion PAR_DER
    {
        printf("goto %s\n", $<cadena>5);
        printf("%s:\n", $<cadena>9);
    }
    bloque
    {
        printf("goto %s\n", $<cadena>10);
        printf("%s:\n", $<cadena>8);
    }
    ;

// condiciones

condicion:
    condicion_or
    {
        strcpy($$, $1);
    }
    ;

condicion_or:
    condicion_and condicion_or_prima
    {
        if($2[0]){
            nuevaTemp($$);
            printf("%s=%s||%s\n", $$, $1, $2);
        }
        else{
            strcpy($$, $1);
        }
    }
    ;

condicion_or_prima:
    OR condicion_and condicion_or_prima
    {
        if($3[0]){
            nuevaTemp($$);
            printf("%s=%s||%s\n", $$, $2, $3);
        }
        else{
            strcpy($$, $2);
        }
    }
    |
    {
        $$[0] = '\0';
    }
    ;

condicion_and:
    comparacion condicion_and_prima
    {
        if($2[0]){
            nuevaTemp($$);
            printf("%s=%s&&%s\n", $$, $1, $2);
        }
        else{
            strcpy($$, $1);
        }
    }
    ;

condicion_and_prima:
    AND comparacion condicion_and_prima
    {
        if($3[0]){
            nuevaTemp($$);
            printf("%s=%s&&%s\n", $$, $2, $3);
        }
        else{
            strcpy($$, $2);
        }
    }
    |
    {
        $$[0] = '\0';
    }
    ;

comparacion:
    expresion operador_relacional expresion
    {
        nuevaTemp($$);
        printf("%s=%s%s%s\n", $$, $1, $2, $3);
    }
    ;

operador_relacional:
    MENOR
    {
        strcpy($$, "<");
    }
    | MAYOR
    {
        strcpy($$, ">");
    }
    | MAYOR_IGUAL
    {
        strcpy($$, ">=");
    }
    | MENOR_IGUAL
    {
        strcpy($$, "<=");
    }
    | IGUAL
    {
        strcpy($$, "==");
    }
    | DIFERENTE
    {
        strcpy($$, "<>");
    }
    ;

%%

// funciones

void nuevaTemp(char *temporal){
    static int actual=1;
    sprintf(temporal,"t%d",actual++);
}

void nuevaEtiqueta(char *etiqueta){
    static int actual=1;
    sprintf(etiqueta,"l%d",actual++);
}

void yyerror(char *mensaje){
    extern int yylineno;
    extern char *yytext;

    fprintf(stderr, "Error de sintaxis en la linea %d: %s, cerca de %s\n",
            yylineno, mensaje, yytext);
}

int main(){
    if(yyparse() == 0){
        printf("Analisis sintactico correcto\n");
    }
    return 0;
}