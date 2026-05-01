/* isr.c - 中断服务程序处理 */

#include "idt.h"
#include "pic.h"
#include "keyboard.h"

extern void print_string(const char *str, char attr);

static const char *exception_messages[] = {
    "Division By Zero", "Debug", "NMI", "Breakpoint",
    "Overflow", "Bounds", "Invalid Opcode", "Coprocessor N/A",
    "Double Fault", "Segment Overrun", "Invalid TSS", "No Segment",
    "Stack Fault", "General Protection", "Page Fault", "Unknown",
    "Coprocessor Fault", "Alignment Check", "Machine Check", "Reserved"
};

void isr_handler(struct registers *regs) {
    print_string("\nEXCEPTION: ", 0x0C);
    if (regs->int_no < 20) {
        print_string(exception_messages[regs->int_no], 0x0C);
    }
    print_string("\n", 0x0C);
    while (1) { __asm__ volatile("hlt"); }
}

void irq_handler(struct registers *regs) {
    pic_send_eoi(regs->int_no - 32);

    /* 键盘中断 (IRQ1) */
    if (regs->int_no == 33) {
        keyboard_handler();
    }
}
