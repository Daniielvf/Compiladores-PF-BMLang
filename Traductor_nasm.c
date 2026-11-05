
#include <stdio.h>
#include <string.h>
#include <ctype.h>

char variables[200][50];
int cantidad = 0;

void registrar(char *nombre){
    char limpio[50];
    int i;

    if(nombre[0] == '$')
        strcpy(limpio, nombre + 1);
    else
        strcpy(limpio, nombre);

    for(i = 0; i < cantidad; i++){
        if(strcmp(variables[i], limpio) == 0)
            return;
    }

    if(cantidad < 200){
        strcpy(variables[cantidad], limpio);
        cantidad++;
    }
}

void buscarVariables(char *linea){
    int i = 0, j;
    char nombre[50];

    while(linea[i] != '\0'){
        if(linea[i] == '$' ||
           (linea[i] == 't' && isdigit((unsigned char)linea[i+1]) &&
            (i == 0 || !isalnum((unsigned char)linea[i-1])))){

            j = 0;

            if(linea[i] == '$')
                i++;

            while(isalnum((unsigned char)linea[i]) && j < 49){
                nombre[j++] = linea[i++];
            }
            nombre[j] = '\0';

            if(j > 0)
                registrar(nombre);
        }
        else{
            i++;
        }
    }
}

void operando(char *resultado, char *valor){
    if(valor[0] == '$'){
        sprintf(resultado, "[%s]", valor + 1);
    }
    else if(valor[0] == 't' && isdigit((unsigned char)valor[1])){
        sprintf(resultado, "[%s]", valor);
    }
    else{
        strcpy(resultado, valor);
    }
}

int main(){
    FILE *entrada;
    FILE *salida;
    char linea[256];
    char destino[50], expresion[100];
    char izquierda[100], derecha[100];
    char op1[110], op2[110];
    char etiqueta[50], valor[50];
    char *simbolo;
    char salto[8];
    int i, errores = 0, numeroComparacion = 1;
    int tamOperador;
    char operador;

    entrada = fopen("codigo_intermedio.txt", "r");
    if(entrada == NULL){
        printf("No se encontro codigo_intermedio.txt\n");
        return 1;
    }

    // Primera lectura: registrar variables y temporales
    while(fgets(linea, sizeof(linea), entrada)){
        buscarVariables(linea);
    }
    rewind(entrada);

    salida = fopen("programa.asm", "w");
    if(salida == NULL){
        fclose(entrada);
        printf("No se pudo crear programa.asm\n");
        return 1;
    }

    fprintf(salida, "section .bss\n");
    for(i = 0; i < cantidad; i++){
        fprintf(salida, "    %s resd 1\n", variables[i]);
    }

    fprintf(salida, "\nsection .text\n");
    fprintf(salida, "    global _start\n\n");
    fprintf(salida, "_start:\n");

    while(fgets(linea, sizeof(linea), entrada)){
        // Ignorar encabezado, mensaje final y lineas vacias
        if(strncmp(linea, "main:", 5) == 0 ||
           strncmp(linea, "Analisis sintactico correcto", 28) == 0 ||
           linea[0] == '\n'){
            continue;
        }

        // Saltar si el resultado de la condicion es cero
        if(strncmp(linea, "ifZ ", 4) == 0){
            if(sscanf(linea, "ifZ %49s goto %49s", valor, etiqueta) == 2){
                operando(op1, valor);
                fprintf(salida, "    mov eax, %s\n", op1);
                fprintf(salida, "    cmp eax, 0\n");
                fprintf(salida, "    je %s\n", etiqueta);
            }
            else{
                printf("Instruccion no soportada: %s", linea);
                errores++;
            }
            continue;
        }

        // Salto incondicional
        if(strncmp(linea, "goto ", 5) == 0){
            if(sscanf(linea, "goto %49s", etiqueta) == 1)
                fprintf(salida, "    jmp %s\n", etiqueta);
            else{
                printf("Instruccion no soportada: %s", linea);
                errores++;
            }
            continue;
        }

        // Etiquetas l1:, l2: ...
        if(sscanf(linea, " %49[^:]%*[: ]", etiqueta) == 1 &&
           etiqueta[0] == 'l' && isdigit((unsigned char)etiqueta[1]) &&
           strchr(linea, ':') != NULL && strchr(linea, '=') == NULL){
            fprintf(salida, "%s:\n", etiqueta);
            continue;
        }

        // Instrucciones de asignacion: destino=expresion
        if(sscanf(linea, " %49[^=]=%99s", destino, expresion) != 2){
            printf("Instruccion no soportada: %s", linea);
            errores++;
            continue;
        }

        // Buscar primero operadores relacionales de dos caracteres
        simbolo = NULL;
        tamOperador = 0;
        for(i = 1; expresion[i] != '\0'; i++){
            if(strncmp(&expresion[i], ">=", 2) == 0){
                simbolo = &expresion[i]; tamOperador = 2; strcpy(salto, "jge"); break;
            }
            if(strncmp(&expresion[i], "<=", 2) == 0){
                simbolo = &expresion[i]; tamOperador = 2; strcpy(salto, "jle"); break;
            }
            if(strncmp(&expresion[i], "==", 2) == 0){
                simbolo = &expresion[i]; tamOperador = 2; strcpy(salto, "je"); break;
            }
            if(strncmp(&expresion[i], "<>", 2) == 0){
                simbolo = &expresion[i]; tamOperador = 2; strcpy(salto, "jne"); break;
            }
            if(expresion[i] == '<'){
                simbolo = &expresion[i]; tamOperador = 1; strcpy(salto, "jl"); break;
            }
            if(expresion[i] == '>'){
                simbolo = &expresion[i]; tamOperador = 1; strcpy(salto, "jg"); break;
            }
        }

        if(simbolo != NULL){
            strcpy(derecha, simbolo + tamOperador);
            *simbolo = '\0';
            strcpy(izquierda, expresion);
            operando(op1, izquierda);
            operando(op2, derecha);

            fprintf(salida, "    mov eax, %s\n", op1);
            fprintf(salida, "    mov ebx, %s\n", op2);
            fprintf(salida, "    cmp eax, ebx\n");
            fprintf(salida, "    %s comp%d_true\n", salto, numeroComparacion);
            fprintf(salida, "    mov eax, 0\n");
            fprintf(salida, "    jmp comp%d_fin\n", numeroComparacion);
            fprintf(salida, "comp%d_true:\n", numeroComparacion);
            fprintf(salida, "    mov eax, 1\n");
            fprintf(salida, "comp%d_fin:\n", numeroComparacion);
            fprintf(salida, "    mov [%s], eax\n",
                    destino[0] == '$' ? destino + 1 : destino);
            numeroComparacion++;
            continue;
        }

        // No traducir operaciones pendientes como &&, || y potencia
        if(strstr(expresion, "&&") != NULL ||
           strstr(expresion, "||") != NULL ||
           strchr(expresion, ':') != NULL){
            printf("Instruccion no soportada: %s", linea);
            errores++;
            continue;
        }

        // Buscar los operadores aritmeticos
        simbolo = NULL;
        for(i = 1; expresion[i] != '\0'; i++){
            if(expresion[i] == '+' || expresion[i] == '-' ||
               expresion[i] == '*' || expresion[i] == '/'){
                simbolo = &expresion[i];
                break;
            }
        }

        if(simbolo == NULL){
            operando(op1, expresion);
            fprintf(salida, "    mov eax, %s\n", op1);
            fprintf(salida, "    mov [%s], eax\n",
                    destino[0] == '$' ? destino + 1 : destino);
        }
        else{
            operador = *simbolo;
            strcpy(derecha, simbolo + 1);
            *simbolo = '\0';
            strcpy(izquierda, expresion);
            operando(op1, izquierda);
            operando(op2, derecha);

            fprintf(salida, "    mov eax, %s\n", op1);
            if(operador == '+'){
                fprintf(salida, "    add eax, %s\n", op2);
            }
            else if(operador == '-'){
                fprintf(salida, "    sub eax, %s\n", op2);
            }
            else if(operador == '*'){
                fprintf(salida, "    mov ebx, %s\n", op2);
                fprintf(salida, "    imul eax, ebx\n");
            }
            else if(operador == '/'){
                fprintf(salida, "    cdq\n");
                fprintf(salida, "    mov ebx, %s\n", op2);
                fprintf(salida, "    idiv ebx\n");
            }
            fprintf(salida, "    mov [%s], eax\n",
                    destino[0] == '$' ? destino + 1 : destino);
        }
    }

    fprintf(salida, "\n    mov eax, 1\n");
    fprintf(salida, "    xor ebx, ebx\n");
    fprintf(salida, "    int 0x80\n");

    fclose(entrada);
    fclose(salida);

    if(errores > 0){
        printf("Hay %d instrucciones pendientes de traducir\n", errores);
        remove("programa.asm");
        return 1;
    }

    printf("Archivo programa.asm generado correctamente\n");
    return 0;
}
