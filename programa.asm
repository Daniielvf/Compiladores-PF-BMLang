section .bss
    a resd 1
    b resd 1
    t1 resd 1
    resultado resd 1
    buffer resb 16
    entrada_char resb 1

section .text
    global _start

_start:
    call leer
    push eax
    pop eax
    mov [a], eax
    call leer
    push eax
    pop eax
    mov [b], eax
    mov eax, [a]
    add eax, [b]
    mov [t1], eax
    mov eax, [t1]
    mov [resultado], eax
    mov eax, [resultado]
    push eax
    pop eax
    call imprimir

    mov eax, 1
    xor ebx, ebx
    int 0x80

imprimir:
    mov edi, buffer+15
    mov byte [edi], 10
    mov esi, buffer+16
    mov ebx, 10
    mov ebp, 0
    cmp eax, 0
    jge imprimir_digitos
    mov ebp, 1
imprimir_digitos:
    cdq
    idiv ebx
    cmp edx, 0
    jge imprimir_positivo
    neg edx
imprimir_positivo:
    add dl, '0'
    dec edi
    mov [edi], dl
    cmp eax, 0
    jne imprimir_digitos
    cmp ebp, 0
    je imprimir_mostrar
    dec edi
    mov byte [edi], '-'
imprimir_mostrar:
    mov eax, 4
    mov ebx, 1
    mov ecx, edi
    mov edx, esi
    sub edx, edi
    int 0x80
    ret

leer:
    mov esi, 0
    mov edi, 1
leer_caracter:
    mov eax, 3
    mov ebx, 0
    mov ecx, entrada_char
    mov edx, 1
    int 0x80
    cmp eax, 0
    jle leer_fin
    mov al, [entrada_char]
    cmp al, 10
    je leer_fin
    cmp al, 13
    je leer_fin
    cmp al, '-'
    jne leer_digito
    mov edi, -1
    jmp leer_caracter
leer_digito:
    cmp al, '0'
    jl leer_caracter
    cmp al, '9'
    jg leer_caracter
    sub al, '0'
    movzx eax, al
    imul esi, esi, 10
    add esi, eax
    jmp leer_caracter
leer_fin:
    mov eax, esi
    imul eax, edi
    ret
