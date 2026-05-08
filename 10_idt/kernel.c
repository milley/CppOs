/* kernel.c - 测试 IDT 和异常处理 */

#include <stdint.h>
#include "idt.h"
#include "pic.h"
#include "allocator.h"

#define VIDEO_MEMORY 0xB8000
#define MAX_COLS 80
#define MAX_ROWS 25

#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define CYAN_ON_BLACK  0x0B
#define RED_ON_BLACK   0x0C

static int cursor_col = 0;
static int cursor_row = 0;

/* 端口 I/O */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* 串口初始化 */
static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

static void serial_putchar(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, c);
}

static void serial_print(const char *str) {
    while (*str) {
        serial_putchar(*str++);
    }
}

/* VGA 输出 */
void print_char(char c) {
    char *video = (char *)VIDEO_MEMORY;
    int offset = (cursor_row * MAX_COLS + cursor_col) * 2;

    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
        return;
    }

    video[offset] = c;
    video[offset + 1] = WHITE_ON_BLACK;

    cursor_col++;
    if (cursor_col >= MAX_COLS) {
        cursor_col = 0;
        cursor_row++;
    }
}

void print_string(const char *str) {
    serial_print(str);
    while (*str) {
        print_char(*str);
        str++;
    }
}

void clear_screen(void) {
    char *video = (char *)VIDEO_MEMORY;
    for (int i = 0; i < MAX_COLS * MAX_ROWS * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = WHITE_ON_BLACK;
    }
    cursor_col = 0;
    cursor_row = 0;
}

void print_hex(uint32_t value) {
    char hex[] = "0123456789ABCDEF";
    char buffer[9];

    buffer[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex[value & 0xF];
        value >>= 4;
    }

    print_string("0x");
    print_string(buffer);
}

void print_decimal(uint32_t value) {
    char buffer[16];
    int i = 0;

    if (value == 0) {
        print_char('0');
        serial_putchar('0');
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        char c = buffer[--i];
        print_char(c);
        serial_putchar(c);
    }
}

/* 定时器中断计数 */
static volatile uint32_t timer_ticks = 0;

/* 定时器中断处理函数 */
void timer_handler(struct interrupt_frame *frame) {
    timer_ticks++;

    /* 每 18 次 tick（约1秒）更新显示 */
    if ((timer_ticks % 18) == 0) {
        /* 简单更新 VGA */
        char *video = (char *)0xB8000;
        int offset = 5 * 160 + 14 * 2;  /* 第 6 行 */

        uint32_t secs = timer_ticks / 18;
        video[offset] = '0' + (secs % 10);
        video[offset + 1] = 0x0B;  /* CYAN */
    }

    /* 发送 EOI */
    pic_send_eoi(0);
}

/* 键盘中断处理函数 */
void keyboard_handler(struct interrupt_frame *frame) {
    uint8_t scancode = inb(0x60);

    /* 显示扫描码 */
    char *video = (char *)VIDEO_MEMORY;
    int offset = 7 * 160;  /* 第 8 行 */

    video[offset] = 'K';
    video[offset + 1] = GREEN_ON_BLACK;
    video[offset + 2] = 'e';
    video[offset + 3] = GREEN_ON_BLACK;
    video[offset + 4] = 'y';
    video[offset + 5] = GREEN_ON_BLACK;
    video[offset + 6] = ':';
    video[offset + 7] = GREEN_ON_BLACK;
    video[offset + 8] = ' ';
    video[offset + 9] = GREEN_ON_BLACK;

    /* 显示扫描码的十六进制 */
    char hex[] = "0123456789ABCDEF";
    video[offset + 10] = hex[(scancode >> 4) & 0xF];
    video[offset + 11] = GREEN_ON_BLACK;
    video[offset + 12] = hex[scancode & 0xF];
    video[offset + 13] = GREEN_ON_BLACK;

    /* 发送 EOI */
    pic_send_eoi(1);
}

/* 测试除零异常 */
void test_divide_error(void) {
    print_string("Test: Triggering Divide Error...\n");

    /* 除零会产生异常 */
    int a = 1;
    int b = 0;
    int c = a / b;

    /* 不会执行到这里 */
    (void)c;
}

/* 测试无效操作码异常 */
void test_invalid_opcode(void) {
    print_string("Test: Triggering Invalid Opcode...\n");

    /* 执行无效指令 */
    __asm__ volatile(".byte 0x06, 0x07");  /* 在 32 位模式下无效的指令 */
}

/* 测试一般保护错误 */
void test_gpf(void) {
    print_string("Test: Triggering General Protection Fault...\n");

    /* 尝试加载无效的选择子 */
    __asm__ volatile("mov $0xFFFFFFFF, %ax; mov %ax, %ds");
}

void kernel_main(void) {
    serial_init();
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - IDT & Exception Demo\n");
    print_string("========================================\n\n");

    /* 初始化分配器 */
    print_string("Step 1: Initialize allocator...\n");
    allocator_init(0x100000, 0x100000);
    print_string("  Done!\n\n");

    /* 初始化 PIC */
    print_string("Step 2: Initialize PIC...\n");
    pic_init();
    print_string("  Done!\n\n");

    /* 初始化 IDT */
    print_string("Step 3: Initialize IDT...\n");
    idt_init();
    print_string("  Done!\n\n");

    /* 注册中断处理函数 */
    print_string("Step 4: Register interrupt handlers...\n");
    idt_register_handler(IRQ_TIMER, timer_handler);
    idt_register_handler(IRQ_KEYBOARD, keyboard_handler);
    print_string("  Timer handler registered\n");
    print_string("  Keyboard handler registered\n\n");

    /* 启用中断 */
    print_string("Step 5: Enable interrupts...\n");
    enable_interrupts();

    /* 启用定时器和键盘中断 */
    pic_unmask_irq(0);  /* 定时器 */
    pic_unmask_irq(1);  /* 键盘 */

    print_string("  Interrupts enabled!\n\n");

    print_string("========================================\n");
    print_string("   System running with interrupts\n");
    print_string("========================================\n\n");

    print_string("Tests available:\n");
    print_string("  1. Divide Error (type 'd')\n");
    print_string("  2. Invalid Opcode (type 'i')\n");
    print_string("  3. General Protection (type 'g')\n\n");

    print_string("Timer is running (see line 6)\n");
    print_string("Keyboard is active (press any key)\n\n");

    /* 主循环 */
    while (1) {
        __asm__ volatile("hlt");
    }
}
