/* kernel.c - 测试系统调用 */

#include <stdint.h>
#include <stddef.h>
#include "idt.h"
#include "pic.h"
#include "syscall.h"
#include "usermode.h"
#include "allocator.h"
#include "process.h"

#define VIDEO_MEMORY 0xB8000
#define MAX_COLS 80

#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define CYAN_ON_BLACK  0x0B
#define RED_ON_BLACK   0x0C

static int cursor_col = 0;
static int cursor_row = 0;

/* 时钟计数 */
uint32_t timer_ticks = 0;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

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
    while (*str) {
        print_char(*str);
        str++;
    }
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

void clear_screen(void) {
    char *video = (char *)VIDEO_MEMORY;
    for (int i = 0; i < MAX_COLS * 25 * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = WHITE_ON_BLACK;
    }
    cursor_col = 0;
    cursor_row = 0;
}

/* 定时器中断处理函数 */
void timer_handler(struct interrupt_frame *frame) {
    timer_ticks++;

    /* 更新时钟显示 */
    char *video = (char *)VIDEO_MEMORY;
    int offset = 2 * 160 + 10 * 2;
    uint32_t secs = timer_ticks / 18;

    video[offset] = '0' + (secs % 10);
    video[offset + 1] = CYAN_ON_BLACK;

    /* 进程调度 */
    struct process *current = process_get_current();
    if (current != NULL) {
        current->ticks_remaining--;
        if (current->ticks_remaining == 0) {
            schedule_from_interrupt(frame);
        }
    }

    pic_send_eoi(0);
}

/* 键盘中断处理函数 */
void keyboard_handler(struct interrupt_frame *frame) {
    uint8_t scancode = inb(0x60);
    (void)scancode;
    pic_send_eoi(1);
}

/* 用户程序 - 使用系统调用 */
void user_program(void) {
    /* 使用系统调用打印信息 */
    sys_puts("Hello from User Mode!");
    sys_puts("Testing syscalls...");

    /* 获取 PID */
    uint32_t pid = sys_getpid();

    /* 显示 PID */
    char *video = (char *)0xB8000;
    int offset = 14 * 160;
    video[offset] = 'P';
    video[offset + 1] = GREEN_ON_BLACK;
    video[offset + 2] = 'I';
    video[offset + 3] = GREEN_ON_BLACK;
    video[offset + 4] = 'D';
    video[offset + 5] = GREEN_ON_BLACK;
    video[offset + 6] = ':';
    video[offset + 7] = GREEN_ON_BLACK;
    video[offset + 8] = ' ';
    video[offset + 9] = GREEN_ON_BLACK;
    video[offset + 10] = '0' + pid;
    video[offset + 11] = GREEN_ON_BLACK;

    /* 循环打印字符 */
    int count = 0;
    while (1) {
        sys_putc('A' + (count % 26));
        sys_putc('\n');

        /* 睡眠约 1 秒 */
        sys_sleep(18);

        count++;
    }
}

/* 测试进程 1 */
void test_process_1(void) {
    uint32_t pid = sys_getpid();
    char *video = (char *)0xB8000;
    int count = 0;

    while (1) {
        /* 在第 5 行显示 PID 和计数 */
        int offset = 5 * 160;
        video[offset] = 'P';
        video[offset + 1] = RED_ON_BLACK;
        video[offset + 2] = '1';
        video[offset + 3] = RED_ON_BLACK;
        video[offset + 4] = ':';
        video[offset + 5] = RED_ON_BLACK;
        video[offset + 6] = '0' + pid;
        video[offset + 7] = RED_ON_BLACK;
        video[offset + 8] = ' ';
        video[offset + 9] = RED_ON_BLACK;
        video[offset + 10] = 'C';
        video[offset + 11] = RED_ON_BLACK;
        video[offset + 12] = '0' + (count % 10);
        video[offset + 13] = RED_ON_BLACK;

        /* 忙等待 - 让出 CPU */
        for (volatile int i = 0; i < 100000; i++);

        count++;
    }
}

/* 测试进程 2 */
void test_process_2(void) {
    uint32_t pid = sys_getpid();
    char *video = (char *)0xB8000;
    int count = 0;

    while (1) {
        /* 在第 6 行显示 PID 和计数 */
        int offset = 6 * 160;
        video[offset] = 'P';
        video[offset + 1] = GREEN_ON_BLACK;
        video[offset + 2] = '2';
        video[offset + 3] = GREEN_ON_BLACK;
        video[offset + 4] = ':';
        video[offset + 5] = GREEN_ON_BLACK;
        video[offset + 6] = '0' + pid;
        video[offset + 7] = GREEN_ON_BLACK;
        video[offset + 8] = ' ';
        video[offset + 9] = GREEN_ON_BLACK;
        video[offset + 10] = 'C';
        video[offset + 11] = GREEN_ON_BLACK;
        video[offset + 12] = '0' + (count % 10);
        video[offset + 13] = GREEN_ON_BLACK;

        /* 忙等待 - 让出 CPU */
        for (volatile int i = 0; i < 100000; i++);

        count++;
    }
}

/* 测试进程 3 */
void test_process_3(void) {
    uint32_t pid = sys_getpid();
    char *video = (char *)0xB8000;
    int count = 0;

    while (1) {
        /* 在第 7 行显示 PID 和计数 */
        int offset = 7 * 160;
        video[offset] = 'P';
        video[offset + 1] = CYAN_ON_BLACK;
        video[offset + 2] = '3';
        video[offset + 3] = CYAN_ON_BLACK;
        video[offset + 4] = ':';
        video[offset + 5] = CYAN_ON_BLACK;
        video[offset + 6] = '0' + pid;
        video[offset + 7] = CYAN_ON_BLACK;
        video[offset + 8] = ' ';
        video[offset + 9] = CYAN_ON_BLACK;
        video[offset + 10] = 'C';
        video[offset + 11] = CYAN_ON_BLACK;
        video[offset + 12] = '0' + (count % 10);
        video[offset + 13] = CYAN_ON_BLACK;

        /* 忙等待 - 让出 CPU */
        for (volatile int i = 0; i < 100000; i++);

        count++;
    }
}

void kernel_main(void) {
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - Multi-Process Demo\n");
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

    /* 初始化用户模式 */
    print_string("Step 4: Initialize user mode...\n");
    usermode_init();
    print_string("  Done!\n\n");

    /* 初始化系统调用 */
    print_string("Step 5: Initialize syscalls...\n");
    syscall_init();
    print_string("  Done!\n\n");

    /* 初始化进程管理 */
    print_string("Step 6: Initialize process manager...\n");
    scheduler_init();
    print_string("  Done!\n\n");

    /* 注册中断处理函数 */
    print_string("Step 7: Register interrupt handlers...\n");
    idt_register_handler(IRQ_TIMER, timer_handler);
    idt_register_handler(IRQ_KEYBOARD, keyboard_handler);
    print_string("  Timer handler registered\n");
    print_string("  Keyboard handler registered\n\n");

    /* 启用中断 */
    print_string("Step 8: Enable interrupts...\n");
    enable_interrupts();
    pic_unmask_irq(0);
    pic_unmask_irq(1);
    print_string("  Done!\n\n");

    /* 显示时钟标签 */
    char *video = (char *)VIDEO_MEMORY;
    int offset = 2 * 160;
    video[offset] = 'T';
    video[offset + 1] = CYAN_ON_BLACK;
    video[offset + 2] = 'i';
    video[offset + 3] = CYAN_ON_BLACK;
    video[offset + 4] = 'c';
    video[offset + 5] = CYAN_ON_BLACK;
    video[offset + 6] = 'k';
    video[offset + 7] = CYAN_ON_BLACK;
    video[offset + 8] = 's';
    video[offset + 9] = CYAN_ON_BLACK;

    print_string("========================================\n");
    print_string("   Creating processes...\n");
    print_string("========================================\n\n");

    /* 创建多个进程 */
    struct process *proc1 = process_create(test_process_1, PRIORITY_NORMAL);
    struct process *proc2 = process_create(test_process_2, PRIORITY_NORMAL);
    struct process *proc3 = process_create(test_process_3, PRIORITY_NORMAL);

    if (proc1) print_string("  Process 1 created (PID: 1)\n");
    if (proc2) print_string("  Process 2 created (PID: 2)\n");
    if (proc3) print_string("  Process 3 created (PID: 3)\n\n");

    print_string("========================================\n");
    print_string("   Starting scheduler...\n");
    print_string("========================================\n\n");

    /* 启动第一个进程 */
    if (proc1) {
        process_set_current(proc1);
        switch_to_first(proc1);
    }

    /* 如果进程切换失败，进入无限循环 */
    while (1) {
        __asm__ volatile("hlt");
    }
}
