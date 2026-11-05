section .bss
    a resd 1
    t1 resd 1
    b resd 1

section .text
    global _start

_start:
    mov eax, 10
    mov [a], eax
    mov eax, [a]
    mov ebx, 5
    cmp eax, ebx
    jg comp1_true
    mov eax, 0
    jmp comp1_fin
comp1_true:
    mov eax, 1
comp1_fin:
    mov [t1], eax
    mov eax, [t1]
    cmp eax, 0
    je l1
    mov eax, 20
    mov [b], eax
    jmp l2
l1:
    mov eax, 30
    mov [b], eax
l2:

    mov eax, 1
    xor ebx, ebx
    int 0x80
