/* idt.h - 中断描述符表头文件 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* IDT 条目数量 */
#define IDT_ENTRIES 256

/* 异常编号 */
#define EXCEPTION_DIVIDE_ERROR    0   /* 除零错误 */
#define EXCEPTION_DEBUG           1   /* 调试异常 */
#define EXCEPTION_NMI             2   /* 不可屏蔽中断 */
#define EXCEPTION_BREAKPOINT      3   /* 断点 */
#define EXCEPTION_OVERFLOW        4   /* 溢出 */
#define EXCEPTION_BOUND_RANGE     5   /* 边界检查 */
#define EXCEPTION_INVALID_OPCODE  6   /* 无效操作码 */
#define EXCEPTION_DEVICE_NOT_AVAIL 7 /* 设备不可用 */
#define EXCEPTION_DOUBLE_FAULT    8   /* 双重错误 */
#define EXCEPTION_COPROCESSOR     9   /* 协处理器段越界 */
#define EXCEPTION_INVALID_TSS     10  /* 无效 TSS */
#define EXCEPTION_SEGMENT_NOT_PRESENT 11 /* 段不存在 */
#define EXCEPTION_STACK_SEGMENT   12  /* 栈段错误 */
#define EXCEPTION_GENERAL_PROTECT 13  /* 一般保护错误 */
#define EXCEPTION_PAGE_FAULT      14  /* 页错误 */
#define EXCEPTION_RESERVED        15  /* 保留 */
#define EXCEPTION_FPU_ERROR       16  /* FPU 错误 */
#define EXCEPTION_ALIGNMENT_CHECK 17  /* 对齐检查 */
#define EXCEPTION_MACHINE_CHECK   18  /* 机器检查 */
#define EXCEPTION_SIMD_ERROR      19  /* SIMD 错误 */

/* 中断向量号（自定义） */
#define IRQ_BASE          32    /* IRQ 基地址 */
#define IRQ_TIMER         (IRQ_BASE + 0)   /* 定时器 */
#define IRQ_KEYBOARD      (IRQ_BASE + 1)   /* 键盘 */
#define IRQ_CASCADE       (IRQ_BASE + 2)   /* 级联 */
#define IRQ_COM2          (IRQ_BASE + 3)   /* 串口2 */
#define IRQ_COM1          (IRQ_BASE + 4)   /* 串口1 */
#define IRQ_LPT2          (IRQ_BASE + 5)   /* 并口2 */
#define IRQ_FLOPPY        (IRQ_BASE + 6)   /* 软盘 */
#define IRQ_LPT1          (IRQ_BASE + 7)   /* 并口1 */
#define IRQ_RTC           (IRQ_BASE + 8)   /* 实时时钟 */
#define IRQ_ACPI          (IRQ_BASE + 9)   /* ACPI */
#define IRQ_MOUSE         (IRQ_BASE + 12)  /* 鼠标 */
#define IRQ_FPU           (IRQ_BASE + 13)  /* FPU */
#define IRQ_ATA_PRIMARY   (IRQ_BASE + 14)  /* ATA 主 */
#define IRQ_ATA_SECONDARY (IRQ_BASE + 15)  /* ATA 从 */

/* 系统调用中断号 */
#define INT_SYSCALL       0x80  /* 系统调用 */

/* IDT 条目结构 */
struct idt_entry {
    uint16_t base_low;     /* 处理函数地址低 16 位 */
    uint16_t selector;     /* 代码段选择子 */
    uint8_t  zero;         /* 保留，必须为 0 */
    uint8_t  type_attr;    /* 类型和属性 */
    uint16_t base_high;    /* 处理函数地址高 16 位 */
} __attribute__((packed));

/* IDT 指针结构 */
struct idt_ptr {
    uint16_t limit;        /* IDT 大小 - 1 */
    uint32_t base;         /* IDT 基地址 */
} __attribute__((packed));

/* 中断栈帧（从栈顶向下） */
struct interrupt_frame {
    uint32_t eax, ecx, edx, ebx, ebp, esi, edi; /* 通用寄存器（手动保存） */
    uint32_t ds, es, fs, gs;  /* 段寄存器（手动保存） */
    uint32_t int_no, err_code; /* 中断号和错误码（手动保存） */
    uint32_t eip, cs, eflags; /* CPU 自动保存 */
    uint32_t useresp, ss;      /* 用户栈（仅特权级变化时） */
} __attribute__((packed));

/* 中断处理函数类型 */
typedef void (*interrupt_handler_t)(struct interrupt_frame *frame);

/* 函数声明 */

/* 初始化 IDT */
void idt_init(void);

/* 设置 IDT 条目 */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t type);

/* 注册中断处理函数 */
void idt_register_handler(uint8_t num, interrupt_handler_t handler);

/* 异常处理函数 */
void exception_handler(struct interrupt_frame *frame);

/* 通用中断处理函数 */
void interrupt_handler(struct interrupt_frame *frame);

/* 启用/禁用中断 */
void enable_interrupts(void);
void disable_interrupts(void);

#endif
