/* idt.c - 中断描述符表实现 */

#include "idt.h"

/* IDT 表 */
static struct idt_entry idt[IDT_ENTRIES];
static struct idt_ptr idt_pointer;

/* 中断处理函数表 */
static interrupt_handler_t interrupt_handlers[IDT_ENTRIES];

/* 异常名称 */
static const char *exception_messages[] = {
    "Divide Error",
    "Debug Exception",
    "NMI Interrupt",
    "Breakpoint",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 FPU Error",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception"
};

/* 外部汇编入口点（在 idt_asm.asm 中定义） */
extern uint32_t idt_handlers[];

/* 设置 IDT 条目 */
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t type) {
    idt[num].base_low  = handler & 0xFFFF;
    idt[num].base_high = (handler >> 16) & 0xFFFF;
    idt[num].selector  = selector;
    idt[num].zero      = 0;
    idt[num].type_attr = type;
}

/* 注册中断处理函数 */
void idt_register_handler(uint8_t num, interrupt_handler_t handler) {
    interrupt_handlers[num] = handler;
}

/* 初始化 IDT */
void idt_init(void) {
    /* 清空 IDT 和处理函数表 */
    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt[i].base_low  = 0;
        idt[i].base_high = 0;
        idt[i].selector  = 0;
        idt[i].zero      = 0;
        idt[i].type_attr = 0;
        interrupt_handlers[i] = 0;
    }

    /* 设置异常处理入口（0-31） */
    for (int i = 0; i < 32; i++) {
        idt_set_gate(i, idt_handlers[i], 0x08, 0x8E);
    }

    /* 设置 IRQ 处理入口（32-47） */
    for (int i = 32; i < 48; i++) {
        idt_set_gate(i, idt_handlers[i], 0x08, 0x8E);
    }

    /* 设置系统调用入口（0x80） */
    idt_set_gate(0x80, idt_handlers[0x80], 0x08, 0x8E);

    /* 加载 IDT */
    idt_pointer.limit = sizeof(idt) - 1;
    idt_pointer.base = (uint32_t)&idt;

    __asm__ volatile("lidt %0" : : "m"(idt_pointer));
}

/* 打印字符串（简单版本） */
static void print_string(const char *str) {
    char *video = (char *)0xB8000;
    static int row = 10;  /* 从第 10 行开始显示异常信息 */

    /* 找到第一个空行 */
    while (row < 25) {
        if (video[row * 160] == ' ') break;
        row++;
    }
    if (row >= 25) row = 10;

    int offset = row * 160;
    for (int i = 0; str[i] && i < 78; i++) {
        video[offset + i * 2] = str[i];
        video[offset + i * 2 + 1] = 0x0C;  /* 红色 */
    }
    row++;
}

/* 打印数字 */
static void print_number(uint32_t num) {
    char buffer[11];
    int i = 0;

    if (num == 0) {
        print_string("0");
        return;
    }

    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }

    char result[12];
    int j = 0;
    while (i > 0) {
        result[j++] = buffer[--i];
    }
    result[j] = '\0';
    print_string(result);
}

/* 打印十六进制 */
static void print_hex(uint32_t num) {
    char hex[] = "0123456789ABCDEF";
    char buffer[11];

    buffer[0] = '0';
    buffer[1] = 'x';

    for (int i = 7; i >= 0; i--) {
        buffer[2 + i] = hex[num & 0xF];
        num >>= 4;
    }
    buffer[10] = '\0';
    print_string(buffer);
}

/* 异常处理函数 */
void exception_handler(struct interrupt_frame *frame) {
    /* 显示异常信息 */
    print_string("EXCEPTION: ");
    if (frame->int_no < 20) {
        print_string(exception_messages[frame->int_no]);
    } else {
        print_string("Unknown Exception");
    }
    print_string("");

    print_string("Interrupt: ");
    print_number(frame->int_no);
    print_string("  Error Code: ");
    print_hex(frame->err_code);

    print_string("EIP: ");
    print_hex(frame->eip);
    print_string("  CS: ");
    print_hex(frame->cs);

    print_string("EFLAGS: ");
    print_hex(frame->eflags);

    /* 如果是页错误，显示导致错误的地址 */
    if (frame->int_no == EXCEPTION_PAGE_FAULT) {
        uint32_t fault_addr;
        __asm__ volatile("mov %%cr2, %0" : "=r"(fault_addr));
        print_string("Fault Address: ");
        print_hex(fault_addr);
    }

    /* 无限循环（挂起系统） */
    while (1) {
        __asm__ volatile("hlt");
    }
}

/* 通用中断处理函数 */
void interrupt_handler(struct interrupt_frame *frame) {
    /* 检查是否有注册的处理函数 */
    if (interrupt_handlers[frame->int_no]) {
        interrupt_handlers[frame->int_no](frame);
    } else if (frame->int_no < 32) {
        /* 未处理的异常 */
        exception_handler(frame);
    }
    /* 其他中断忽略 */
}

/* 启用中断 */
void enable_interrupts(void) {
    __asm__ volatile("sti");
}

/* 禁用中断 */
void disable_interrupts(void) {
    __asm__ volatile("cli");
}
