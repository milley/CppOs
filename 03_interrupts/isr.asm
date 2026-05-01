; isr.asm - 中断服务程序通用入口

[BITS 32]

[EXTERN isr_handler]
[EXTERN irq_handler]

; ISR 通用处理程序
global isr_common_stub
isr_common_stub:
    ; 保存寄存器
    pusha
    mov ax, ds
    push eax

    ; 设置内核数据段
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 调用 C 处理函数
    push esp            ; 传递栈指针
    call isr_handler
    add esp, 4

    ; 恢复寄存器
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa

    ; 清理栈上的错误码和中断号
    add esp, 8

    ; 中断返回
    iret

; IRQ 通用处理程序
global irq_common_stub
irq_common_stub:
    ; 保存寄存器
    pusha
    mov ax, ds
    push eax

    ; 设置内核数据段
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; 调用 C 处理函数
    push esp
    call irq_handler
    add esp, 4

    ; 恢复寄存器
    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    popa

    ; 清理栈
    add esp, 8

    ; 中断返回
    iret
