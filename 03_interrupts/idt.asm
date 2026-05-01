; idt.asm - 中断描述符表定义

[BITS 32]

; 外部中断处理函数
[EXTERN isr_common_stub]
[EXTERN irq_common_stub]

; CPU 异常处理程序（0-31）
%macro ISR_NOERR 1
    global isr%1
isr%1:
    push dword 0            ; 无错误码，压入0
    push dword %1           ; 压入中断号
    jmp isr_common_stub
%endmacro

%macro ISR_ERR 1
    global isr%1
isr%1:
    push dword %1           ; 有错误码，直接压入中断号
    jmp isr_common_stub
%endmacro

; IRQ 处理程序（32-47）
%macro IRQ 2
    global irq%1
irq%1:
    push dword 0
    push dword %2           ; IRQ 号
    jmp irq_common_stub
%endmacro

; CPU 异常（0-31）
ISR_NOERR 0     ; 除零异常
ISR_NOERR 1     ; 调试异常
ISR_NOERR 2     ; NMI
ISR_NOERR 3     ; 断点
ISR_NOERR 4     ; 溢出
ISR_NOERR 5     ; 边界检查
ISR_NOERR 6     ; 无效操作码
ISR_NOERR 7     ; 设备不可用
ISR_ERR   8     ; 双重错误
ISR_NOERR 9     ; 协处理器段越界
ISR_ERR   10    ; 无效TSS
ISR_ERR   11    ; 段不存在
ISR_ERR   12    ; 栈段错误
ISR_ERR   13    ; 一般保护错误
ISR_ERR   14    ; 页错误
ISR_NOERR 15    ; 保留
ISR_NOERR 16    ; x87 FPU错误
ISR_ERR   17    ; 对齐检查
ISR_NOERR 18    ; 机器检查
ISR_NOERR 19    ; SIMD浮点异常
ISR_NOERR 20    ; 虚拟化异常
ISR_NOERR 21    ; 控制保护异常
ISR_NOERR 22    ; 保留
ISR_NOERR 23    ; 保留
ISR_NOERR 24    ; 保留
ISR_NOERR 25    ; 保留
ISR_NOERR 26    ; 保留
ISR_NOERR 27    ; 保留
ISR_NOERR 28    ; 保留
ISR_NOERR 29    ; 保留
ISR_ERR   30    ; 安全异常
ISR_NOERR 31    ; 保留

; IRQ（32-47）
IRQ 0,  32      ; 定时器
IRQ 1,  33      ; 键盘
IRQ 2,  34      ; 级联
IRQ 3,  35      ; COM2
IRQ 4,  36      ; COM1
IRQ 5,  37      ; LPT2
IRQ 6,  38      ; 软盘
IRQ 7,  39      ; LPT1
IRQ 8,  40      ; RTC
IRQ 9,  41      ; ACPI
IRQ 10, 42      ; 可用
IRQ 11, 43      ; 可用
IRQ 12, 44      ; PS/2鼠标
IRQ 13, 45      ; FPU
IRQ 14, 46      ; 主IDE
IRQ 15, 47      ; 从IDE
