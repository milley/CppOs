/* idt.h - 中断描述符表头文件 */

#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* IDT 入口结构 */
struct idt_entry {
    uint16_t base_low;      /* 处理函数地址低16位 */
    uint16_t selector;      /* 代码段选择子 */
    uint8_t  zero;          /* 保留，必须为0 */
    uint8_t  type_attr;     /* 类型和属性 */
    uint16_t base_high;     /* 处理函数地址高16位 */
} __attribute__((packed));

/* IDT 指针结构 */
struct idt_ptr {
    uint16_t limit;         /* IDT大小-1 */
    uint32_t base;          /* IDT地址 */
} __attribute__((packed));

/* 中断栈帧 */
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

/* 函数声明 */
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t sel, uint8_t flags);

/* ISR 声明 (0-31) */
extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);

/* IRQ 声明 (32-47) */
extern void irq0(void);  extern void irq1(void);  extern void irq2(void);
extern void irq3(void);  extern void irq4(void);  extern void irq5(void);
extern void irq6(void);  extern void irq7(void);  extern void irq8(void);
extern void irq9(void);  extern void irq10(void); extern void irq11(void);
extern void irq12(void); extern void irq13(void); extern void irq14(void);
extern void irq15(void);

#endif
