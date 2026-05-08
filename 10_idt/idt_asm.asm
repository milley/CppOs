; idt_asm.asm - IDT 汇编入口点

[BITS 32]

; 外部 C 函数
extern interrupt_handler

; 导出 IDT 处理函数地址数组
global idt_handlers

; 通用中断处理宏
%macro ISR_NOERRCODE 1
    global isr%1
    isr%1:
        push dword 0            ; 无错误码，压入 0
        push dword %1           ; 压入中断号
        jmp isr_common
%endmacro

%macro ISR_ERRCODE 1
    global isr%1
    isr%1:
        push dword %1           ; 压入中断号（错误码已由 CPU 压入）
        jmp isr_common
%endmacro

; 通用中断处理入口
isr_common:
    ; 保存段寄存器
    push gs
    push fs
    push es
    push ds

    ; 保存通用寄存器（不保存 esp）
    push edi
    push esi
    push ebp
    push ebx
    push edx
    push ecx
    push eax

    ; 加载内核数据段
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 调用 C 处理函数
    push esp                ; 传递当前栈指针作为参数
    call interrupt_handler
    add esp, 4              ; 清理参数

    ; 恢复通用寄存器
    pop eax
    pop ecx
    pop edx
    pop ebx
    pop ebp
    pop esi
    pop edi

    ; 恢复段寄存器
    pop ds
    pop es
    pop fs
    pop gs

    ; 清理错误码和中断号
    add esp, 8

    ; 中断返回
    iret

; 异常处理入口（0-31）
; 有错误码的异常：8, 10, 11, 12, 13, 14, 17
ISR_NOERRCODE 0     ; Divide Error
ISR_NOERRCODE 1     ; Debug Exception
ISR_NOERRCODE 2     ; NMI Interrupt
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; BOUND Range Exceeded
ISR_NOERRCODE 6     ; Invalid Opcode
ISR_NOERRCODE 7     ; Device Not Available
ISR_ERRCODE   8     ; Double Fault
ISR_NOERRCODE 9     ; Coprocessor Segment Overrun
ISR_ERRCODE   10    ; Invalid TSS
ISR_ERRCODE   11    ; Segment Not Present
ISR_ERRCODE   12    ; Stack Segment Fault
ISR_ERRCODE   13    ; General Protection Fault
ISR_ERRCODE   14    ; Page Fault
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 FPU Error
ISR_ERRCODE   17    ; Alignment Check
ISR_NOERRCODE 18    ; Machine Check
ISR_NOERRCODE 19    ; SIMD Floating-Point Exception

; 保留异常（20-31）
ISR_NOERRCODE 20
ISR_NOERRCODE 21
ISR_NOERRCODE 22
ISR_NOERRCODE 23
ISR_NOERRCODE 24
ISR_NOERRCODE 25
ISR_NOERRCODE 26
ISR_NOERRCODE 27
ISR_NOERRCODE 28
ISR_NOERRCODE 29
ISR_NOERRCODE 30
ISR_NOERRCODE 31

; IRQ 处理入口（32-47）
ISR_NOERRCODE 32    ; Timer
ISR_NOERRCODE 33    ; Keyboard
ISR_NOERRCODE 34    ; Cascade
ISR_NOERRCODE 35    ; COM2
ISR_NOERRCODE 36    ; COM1
ISR_NOERRCODE 37    ; LPT2
ISR_NOERRCODE 38    ; Floppy
ISR_NOERRCODE 39    ; LPT1
ISR_NOERRCODE 40    ; RTC
ISR_NOERRCODE 41    ; ACPI
ISR_NOERRCODE 42    ; Available
ISR_NOERRCODE 43    ; Available
ISR_NOERRCODE 44    ; Mouse
ISR_NOERRCODE 45    ; FPU
ISR_NOERRCODE 46    ; ATA Primary
ISR_NOERRCODE 47    ; ATA Secondary

; 系统调用入口（0x80）
ISR_NOERRCODE 0x80

; IDT 处理函数地址数组
idt_handlers:
    dd isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7
    dd isr8, isr9, isr10, isr11, isr12, isr13, isr14, isr15
    dd isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23
    dd isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    dd isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39
    dd isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
    ; 填充 48-127
    times 128-48 dd 0
    ; 系统调用在 0x80 (128)
    dd isr0x80
    ; 填充剩余 129-255
    times 255-128 dd 0
