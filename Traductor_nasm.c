
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
    int numeroLogico = 1, numeroPotencia = 1;
    int tamOperador;
    int escriturasPendientes = 0;
    int lecturasPendientes = 0;
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

    fprintf(salida, "    buffer resb 16\n");
    fprintf(salida, "    entrada_char resb 1\n");

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

        // READ: el codigo intermedio genera call input; y pop $a;
        if(strncmp(linea, "call input;", 11) == 0){
            fprintf(salida, "    call leer\n");
            fprintf(salida, "    push eax\n");
            lecturasPendientes++;
            continue;
        }

        if(strncmp(linea, "pop ", 4) == 0){
            if(lecturasPendientes > 0 &&
               sscanf(linea, "pop %49[^;];", valor) == 1){
                fprintf(salida, "    pop eax\n");
                fprintf(salida, "    mov [%s], eax\n",
                        valor[0] == '$' ? valor + 1 : valor);
                lecturasPendientes--;
            }
            else{
                printf("READ incompleto: %s", linea);
                errores++;
            }
            continue;
        }

        // WRITE: el codigo intermedio genera push $a; y call output;
        if(strncmp(linea, "push ", 5) == 0){
            if(sscanf(linea, "push %49[^;];", valor) == 1){
                operando(op1, valor);
                fprintf(salida, "    mov eax, %s\n", op1);
                fprintf(salida, "    push eax\n");
                escriturasPendientes++;
            }
            else{
                printf("Instruccion no soportada: %s", linea);
                errores++;
            }
            continue;
        }

        if(strncmp(linea, "call output;", 12) == 0){
            if(escriturasPendientes > 0){
                fprintf(salida, "    pop eax\n");
                fprintf(salida, "    call imprimir\n");
                escriturasPendientes--;
            }
            else{
                printf("WRITE sin push anterior: %s", linea);
                errores++;
            }
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

        // Operadores logicos: && (AND) y || (OR)
        simbolo = strstr(expresion, "&&");
        if(simbolo == NULL)
            simbolo = strstr(expresion, "||");

        if(simbolo != NULL){
            operador = *simbolo;
            strcpy(derecha, simbolo + 2);
            *simbolo = '\0';
            strcpy(izquierda, expresion);
            operando(op1, izquierda);
            operando(op2, derecha);

            if(operador == '&'){
                // AND: ambos valores deben ser diferentes de cero
                fprintf(salida, "    mov eax, %s\n", op1);
                fprintf(salida, "    cmp eax, 0\n");
                fprintf(salida, "    je log%d_falso\n", numeroLogico);
                fprintf(salida, "    mov eax, %s\n", op2);
                fprintf(salida, "    cmp eax, 0\n");
                fprintf(salida, "    je log%d_falso\n", numeroLogico);
                fprintf(salida, "    mov eax, 1\n");
                fprintf(salida, "    jmp log%d_fin\n", numeroLogico);
                fprintf(salida, "log%d_falso:\n", numeroLogico);
                fprintf(salida, "    mov eax, 0\n");
            }
            else{
                // OR: basta con que uno sea diferente de cero
                fprintf(salida, "    mov eax, %s\n", op1);
                fprintf(salida, "    cmp eax, 0\n");
                fprintf(salida, "    jne log%d_verdadero\n", numeroLogico);
                fprintf(salida, "    mov eax, %s\n", op2);
                fprintf(salida, "    cmp eax, 0\n");
                fprintf(salida, "    jne log%d_verdadero\n", numeroLogico);
                fprintf(salida, "    mov eax, 0\n");
                fprintf(salida, "    jmp log%d_fin\n", numeroLogico);
                fprintf(salida, "log%d_verdadero:\n", numeroLogico);
                fprintf(salida, "    mov eax, 1\n");
            }
            fprintf(salida, "log%d_fin:\n", numeroLogico);
            fprintf(salida, "    mov [%s], eax\n",
                    destino[0] == '$' ? destino + 1 : destino);
            numeroLogico++;
            continue;
        }

        // Potencia: base:exponente, usando multiplicaciones repetidas
        simbolo = strchr(expresion, ':');
        if(simbolo != NULL){
            strcpy(derecha, simbolo + 1);
            *simbolo = '\0';
            strcpy(izquierda, expresion);
            operando(op1, izquierda);
            operando(op2, derecha);

            fprintf(salida, "    mov ebx, %s\n", op1);
            fprintf(salida, "    mov ecx, %s\n", op2);
            fprintf(salida, "    mov eax, 1\n");
            // Para exponentes negativos el resultado entero sera cero
            fprintf(salida, "    cmp ecx, 0\n");
            fprintf(salida, "    jl pot%d_negativo\n", numeroPotencia);
            fprintf(salida, "pot%d_inicio:\n", numeroPotencia);
            fprintf(salida, "    cmp ecx, 0\n");
            fprintf(salida, "    je pot%d_fin\n", numeroPotencia);
            fprintf(salida, "    imul eax, ebx\n");
            fprintf(salida, "    dec ecx\n");
            fprintf(salida, "    jmp pot%d_inicio\n", numeroPotencia);
            fprintf(salida, "pot%d_negativo:\n", numeroPotencia);
            fprintf(salida, "    mov eax, 0\n");
            fprintf(salida, "pot%d_fin:\n", numeroPotencia);
            fprintf(salida, "    mov [%s], eax\n",
                    destino[0] == '$' ? destino + 1 : destino);
            numeroPotencia++;
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

    // Rutina para imprimir el numero de EAX y un salto de linea
    fprintf(salida, "\nimprimir:\n");
    fprintf(salida, "    mov edi, buffer+15\n");
    fprintf(salida, "    mov byte [edi], 10\n");
    fprintf(salida, "    mov esi, buffer+16\n");
    fprintf(salida, "    mov ebx, 10\n");
    fprintf(salida, "    mov ebp, 0\n");
    fprintf(salida, "    cmp eax, 0\n");
    fprintf(salida, "    jge imprimir_digitos\n");
    fprintf(salida, "    mov ebp, 1\n");
    fprintf(salida, "imprimir_digitos:\n");
    fprintf(salida, "    cdq\n");
    fprintf(salida, "    idiv ebx\n");
    fprintf(salida, "    cmp edx, 0\n");
    fprintf(salida, "    jge imprimir_positivo\n");
    fprintf(salida, "    neg edx\n");
    fprintf(salida, "imprimir_positivo:\n");
    fprintf(salida, "    add dl, '0'\n");
    fprintf(salida, "    dec edi\n");
    fprintf(salida, "    mov [edi], dl\n");
    fprintf(salida, "    cmp eax, 0\n");
    fprintf(salida, "    jne imprimir_digitos\n");
    fprintf(salida, "    cmp ebp, 0\n");
    fprintf(salida, "    je imprimir_mostrar\n");
    fprintf(salida, "    dec edi\n");
    fprintf(salida, "    mov byte [edi], '-'\n");
    fprintf(salida, "imprimir_mostrar:\n");
    fprintf(salida, "    mov eax, 4\n");
    fprintf(salida, "    mov ebx, 1\n");
    fprintf(salida, "    mov ecx, edi\n");
    fprintf(salida, "    mov edx, esi\n");
    fprintf(salida, "    sub edx, edi\n");
    fprintf(salida, "    int 0x80\n");
    fprintf(salida, "    ret\n");

    // Rutina para leer un entero desde teclado (positivo o negativo)
    // Devuelve el numero en EAX. La entrada termina con Enter.
    fprintf(salida, "\nleer:\n");
    fprintf(salida, "    mov esi, 0\n");
    fprintf(salida, "    mov edi, 1\n");
    fprintf(salida, "leer_caracter:\n");
    fprintf(salida, "    mov eax, 3\n");
    fprintf(salida, "    mov ebx, 0\n");
    fprintf(salida, "    mov ecx, entrada_char\n");
    fprintf(salida, "    mov edx, 1\n");
    fprintf(salida, "    int 0x80\n");
    fprintf(salida, "    cmp eax, 0\n");
    fprintf(salida, "    jle leer_fin\n");
    fprintf(salida, "    mov al, [entrada_char]\n");
    fprintf(salida, "    cmp al, 10\n");
    fprintf(salida, "    je leer_fin\n");
    fprintf(salida, "    cmp al, 13\n");
    fprintf(salida, "    je leer_fin\n");
    fprintf(salida, "    cmp al, '-'\n");
    fprintf(salida, "    jne leer_digito\n");
    fprintf(salida, "    mov edi, -1\n");
    fprintf(salida, "    jmp leer_caracter\n");
    fprintf(salida, "leer_digito:\n");
    fprintf(salida, "    cmp al, '0'\n");
    fprintf(salida, "    jl leer_caracter\n");
    fprintf(salida, "    cmp al, '9'\n");
    fprintf(salida, "    jg leer_caracter\n");
    fprintf(salida, "    sub al, '0'\n");
    fprintf(salida, "    movzx eax, al\n");
    fprintf(salida, "    imul esi, esi, 10\n");
    fprintf(salida, "    add esi, eax\n");
    fprintf(salida, "    jmp leer_caracter\n");
    fprintf(salida, "leer_fin:\n");
    fprintf(salida, "    mov eax, esi\n");
    fprintf(salida, "    imul eax, edi\n");
    fprintf(salida, "    ret\n");

    if(lecturasPendientes > 0){
        printf("Hay instrucciones READ incompletas\n");
        errores++;
    }

    if(escriturasPendientes > 0){
        printf("Hay instrucciones WRITE incompletas\n");
        errores++;
    }

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
