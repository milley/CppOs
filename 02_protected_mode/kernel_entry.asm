; kernel_entry.asm - 内核入口（汇编）
; 负责调用 C 语言内核

[BITS 32]
[GLOBAL _start]
[EXTERN kernel_main]

section .text
_start:
    ; 调用 C 内核
    call kernel_main

    ; 停机
    hlt
    jmp $
