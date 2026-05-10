; context_switch.asm - 上下文切换汇编实现

[BITS 32]

; 外部函数
extern tss_set_kernel_stack

; 切换到第一个进程
; 这个函数用于从内核态切换到第一个用户进程
; void switch_to_first(struct process *proc)
;
; struct process 布局 (参考 process.h):
;   0:  pid (4)
;   4:  state (4)
;   8:  priority (4)
;   12: context (interrupt_frame, 72 bytes)
;       12: eax (4)
;       16: ecx (4)
;       20: edx (4)
;       24: ebx (4)
;       28: ebp (4)
;       32: esi (4)
;       36: edi (4)
;       40: ds (4)
;       44: es (4)
;       48: fs (4)
;       52: gs (4)
;       56: int_no (4)
;       60: err_code (4)
;       64: eip (4)
;       68: cs (4)
;       72: eflags (4)
;       76: useresp (4)
;       80: ss (4)
;   84: kernel_stack (4)
;   88: user_stack (4)
;   ...

global switch_to_first
switch_to_first:
    mov eax, [esp + 4]          ; proc 指针

    ; 更新 TSS 内核栈
    push dword [eax + 84]       ; proc->kernel_stack
    call tss_set_kernel_stack
    add esp, 4

    mov eax, [esp + 4]          ; 重新获取 proc

    ; 设置内核栈指针 (用于中断返回)
    mov esp, [eax + 84]         ; proc->kernel_stack

    ; 在内核栈上构建中断返回帧
    ; 顺序: ss, useresp, eflags, cs, eip
    push dword [eax + 80]       ; context.ss
    push dword [eax + 76]       ; context.useresp
    push dword [eax + 72]       ; context.eflags
    push dword [eax + 68]       ; context.cs
    push dword [eax + 64]       ; context.eip

    ; 设置段寄存器
    mov bx, [eax + 40]          ; context.ds
    mov ds, bx
    mov bx, [eax + 44]          ; context.es
    mov es, bx
    mov bx, [eax + 48]          ; context.fs
    mov fs, bx
    mov bx, [eax + 52]          ; context.gs
    mov gs, bx

    ; 恢复通用寄存器
    mov edi, [eax + 36]         ; context.edi
    mov esi, [eax + 32]         ; context.esi
    mov ebp, [eax + 28]         ; context.ebp
    mov ebx, [eax + 24]         ; context.ebx
    mov edx, [eax + 20]         ; context.edx
    mov ecx, [eax + 16]         ; context.ecx
    mov eax, [eax + 12]         ; context.eax

    ; iret 返回用户模式
    iret
