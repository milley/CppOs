; usermode.asm - 切换到用户模式

[BITS 32]

; void jump_user_mode(uint32_t entry, uint32_t stack)
global jump_user_mode

jump_user_mode:
    ; 参数: [esp+4] = entry, [esp+8] = stack

    ; 保存 entry 到 esi（跨平台寄存器）
    mov esi, [esp + 4]   ; entry
    mov edi, [esp + 8]   ; stack

    ; 禁用中断（用户模式下没有中断处理程序）
    cli

    ; 设置数据段为用户数据段
    mov ax, 0x23         ; 用户数据段选择子 (0x20 | 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 在栈上设置 IRET 的返回信息
    ; IRET 弹出顺序: EIP, CS, EFLAGS, ESP, SS

    push 0x23            ; SS (用户数据段)
    push edi             ; ESP (用户栈顶)
    push 0x002           ; EFLAGS (IF=0, 关中断)
    push 0x1B            ; CS (用户代码段)
    push esi             ; EIP (入口点)

    ; 执行 IRET，切换到用户模式
    iret
