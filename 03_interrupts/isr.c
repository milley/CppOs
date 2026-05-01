/* isr.c - 中断服务程序处理 */

#include "idt.h"
#include "pic.h"

/* 异常名称 */
static const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

/* 外部打印函数 */
extern void print_string(const char *str, char attr);

/* ISR 处理函数 */
void isr_handler(struct registers *regs) {
    print_string("\nEXCEPTION: ", 0x0C);
    print_string(exception_messages[regs->int_no], 0x0C);
    print_string("\n", 0x0C);

    while (1) {
        __asm__ volatile("hlt");
    }
}

/* IRQ 处理函数 */
void irq_handler(struct registers *regs) {
    pic_send_eoi(regs->int_no - 32);

    /* 定时器中断 */
    if (regs->int_no == 32) {
        extern void timer_handler(void);
        timer_handler();
    }
}
