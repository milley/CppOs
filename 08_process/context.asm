; context.asm - 上下文切换实现

[BITS 32]

; 保存当前进程的栈指针，加载新进程的栈指针
; void context_switch(uint32_t *old_sp, uint32_t new_sp)
global context_switch

context_switch:
    ; 参数:
    ; [esp+4] = old_sp 指针（保存当前栈指针）
    ; [esp+8] = new_sp（新进程的栈指针）

    ; 保存所有通用寄存器
    push ebp
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax

    ; 此时栈布局：
    ; esp+0  = eax
    ; esp+4  = ebx
    ; esp+8  = ecx
    ; esp+12 = edx
    ; esp+16 = esi
    ; esp+20 = edi
    ; esp+24 = ebp
    ; esp+28 = return address
    ; esp+32 = old_sp (argument 1)
    ; esp+36 = new_sp (argument 2)

    ; 保存当前 esp 到 old_sp
    mov eax, [esp + 32]    ; old_sp 指针
    mov [eax], esp         ; 保存当前栈指针

    ; 加载新栈指针
    mov esp, [esp + 36]    ; new_sp

    ; 恢复新进程的寄存器
    pop eax
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi
    pop ebp

    ; 返回（新进程的返回地址在栈上）
    ret