; user_program.asm - 用户程序（最简单版本）

[BITS 32]
[GLOBAL user_program_asm]

user_program_asm:
    ; 在第三行显示 "USER MODE (Ring 3)"
    mov eax, 0xB8000
    add eax, 320           ; 第三行 (160 * 2)

    mov byte [eax], 'U'
    mov byte [eax + 1], 0x0A   ; 绿色
    mov byte [eax + 2], 'S'
    mov byte [eax + 3], 0x0A
    mov byte [eax + 4], 'E'
    mov byte [eax + 5], 0x0A
    mov byte [eax + 6], 'R'
    mov byte [eax + 7], 0x0A
    mov byte [eax + 8], ' '
    mov byte [eax + 9], 0x0A
    mov byte [eax + 10], 'M'
    mov byte [eax + 11], 0x0A
    mov byte [eax + 12], 'O'
    mov byte [eax + 13], 0x0A
    mov byte [eax + 14], 'D'
    mov byte [eax + 15], 0x0A
    mov byte [eax + 16], 'E'
    mov byte [eax + 17], 0x0A

    ; 无限循环（不使用 hlt）
.halt:
    jmp .halt
