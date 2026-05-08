/* kernel.c - 测试进程管理 */

#include <stdint.h>
#include "process.h"
#include "allocator.h"

#define VIDEO_MEMORY 0xB8000
#define SERIAL_PORT  0x3F8
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
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x03);
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);
}

static void serial_putchar(char c) {
    while ((inb(SERIAL_PORT + 5) & 0x20) == 0);
    outb(SERIAL_PORT, c);
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
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        print_char(buffer[--i]);
    }
}

/* 延迟函数 */
static void delay(int count) {
    for (volatile int i = 0; i < count * 100000; i++);
}

/* 进程 A */
void process_a(void) {
    char *video = (char *)VIDEO_MEMORY;

    while (1) {
        /* 在屏幕左上角显示 'A' */
        video[0] = 'A';
        video[1] = GREEN_ON_BLACK;

        delay(5);

        /* 主动让出 CPU */
        print_string("[A] Yielding CPU\n");
        process_schedule();
    }
}

/* 进程 B */
void process_b(void) {
    char *video = (char *)VIDEO_MEMORY;

    while (1) {
        /* 在屏幕第二行显示 'B' */
        video[160] = 'B';
        video[161] = CYAN_ON_BLACK;

        delay(5);

        /* 主动让出 CPU */
        print_string("[B] Yielding CPU\n");
        process_schedule();
    }
}

/* 进程 C */
void process_c(void) {
    char *video = (char *)VIDEO_MEMORY;
    int count = 0;

    while (1) {
        /* 在屏幕第三行显示 'C' 和计数 */
        video[320] = 'C';
        video[321] = RED_ON_BLACK;
        video[322] = ':';
        video[323] = RED_ON_BLACK;
        video[324] = '0' + (count % 10);
        video[325] = RED_ON_BLACK;

        count++;

        delay(3);

        /* 主动让出 CPU */
        print_string("[C] Yielding CPU (count=");
        print_decimal(count);
        print_string(")\n");
        process_schedule();
    }
}

void kernel_main(void) {
    serial_init();
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - Process Management Demo\n");
    print_string("========================================\n\n");

    /* 初始化分配器 */
    print_string("Step 1: Initialize allocator...\n");
    allocator_init(0x100000, 0x100000);
    print_string("  Done!\n\n");

    /* 初始化进程管理 */
    print_string("Step 2: Initialize process manager...\n");
    process_init();
    print_string("  Done!\n\n");

    /* 创建进程 */
    print_string("Step 3: Create processes...\n");

    process_t *proc_a = process_create("ProcessA", process_a, PRIORITY_HIGH);
    if (proc_a) {
        print_string("  Created Process A (PID: ");
        print_decimal(proc_a->pid);
        print_string(", Priority: HIGH)\n");
    }

    process_t *proc_b = process_create("ProcessB", process_b, PRIORITY_NORMAL);
    if (proc_b) {
        print_string("  Created Process B (PID: ");
        print_decimal(proc_b->pid);
        print_string(", Priority: NORMAL)\n");
    }

    process_t *proc_c = process_create("ProcessC", process_c, PRIORITY_LOW);
    if (proc_c) {
        print_string("  Created Process C (PID: ");
        print_decimal(proc_c->pid);
        print_string(", Priority: LOW)\n");
    }

    print_string("\n");

    /* 显示进程信息 */
    print_string("Step 4: Process information\n");
    print_string("----------------------------------------\n");
    print_string("  Total processes: ");
    print_decimal(process_get_count());
    print_string("\n\n");

    /* 开始调度 */
    print_string("Step 5: Start scheduling...\n");
    print_string("----------------------------------------\n");
    print_string("  Processes will alternate execution\n");
    print_string("  Watch for A, B, C on screen\n\n");

    print_string("========================================\n");
    print_string("   Scheduler started!\n");
    print_string("========================================\n\n");

    /* 开始第一个进程 */
    process_schedule();

    /* 永远不会到这里 */
    while (1) {
        __asm__ volatile("hlt");
    }
}
