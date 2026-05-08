; context.asm - 上下文切换实现
;
; 调用约定 (cdecl):
;   - eax, ecx, edx: 调用者保存
;   - ebx, esi, edi, ebp: 被调用者保存
;
; 我们需要保存所有寄存器以确保安全

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

    ; 保存段寄存器（可选，但在内核态可能需要）
    push ds
    push es
    push fs
    push gs

    ; 此时栈布局：
    ; esp+0  = gs
    ; esp+4  = fs
    ; esp+8  = es
    ; esp+12 = ds
    ; esp+16 = eax
    ; esp+20 = ebx
    ; esp+24 = ecx
    ; esp+28 = edx
    ; esp+32 = esi
    ; esp+36 = edi
    ; esp+40 = ebp
    ; esp+44 = return address
    ; esp+48 = old_sp (argument 1)
    ; esp+52 = new_sp (argument 2)

    ; 保存当前 esp 到 old_sp
    mov eax, [esp + 48]    ; old_sp 指针
    mov [eax], esp         ; 保存当前栈指针

    ; 加载新栈指针
    mov esp, [esp + 52]    ; new_sp

    ; 恢复段寄存器
    pop gs
    pop fs
    pop es
    pop ds

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