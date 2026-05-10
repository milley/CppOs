; usermode.asm - 切换到用户模式

[BITS 32]

global jump_user_mode

jump_user_mode:
    mov esi, [esp + 4]   ; entry
    mov edi, [esp + 8]   ; stack

    cli

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23
    push edi
    push 0x202           ; eflags: IF=1 (bit 9), reserved bit 1=1
    push 0x1B
    push esi

    iret
