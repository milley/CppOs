; simple_entry.asm - 简单入口

[BITS 32]
[GLOBAL _start]
[EXTERN kernel_main]

section .text
_start:
    call kernel_main
    hlt
    jmp $
