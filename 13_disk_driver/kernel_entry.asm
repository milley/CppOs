; kernel_entry.asm - 内核入口

[BITS 32]
[GLOBAL _start]
[EXTERN kernel_main]
[EXTERN __bss_start]
[EXTERN __bss_end]

section .text
_start:
    ; 清零 .bss 段
    mov edi, dword [__bss_start]
    mov ecx, dword [__bss_end]
    sub ecx, edi
    shr ecx, 2          ; 除以 4 (按 DWORD 清零)
    xor eax, eax
    rep stosd

    call kernel_main
    hlt
    jmp $
