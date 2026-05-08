/* kernel.c - 测试用户模式 */

#include <stdint.h>
#include "usermode.h"
#include "allocator.h"

#define VIDEO_MEMORY 0xB8000
#define SERIAL_PORT  0x3F8
#define MAX_COLS 80
#define MAX_ROWS 25

#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define CYAN_ON_BLACK  0x0B
#define RED_ON_BLACK   0x0C

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
    int offset = (0 * MAX_COLS + 0) * 2;  /* 固定在左上角 */

    video[offset] = c;
    video[offset + 1] = WHITE_ON_BLACK;
}

void print_string(const char *str) {
    serial_print(str);
}

void clear_screen(void) {
    char *video = (char *)VIDEO_MEMORY;
    for (int i = 0; i < MAX_COLS * MAX_ROWS * 2; i += 2) {
        video[i] = ' ';
        video[i + 1] = WHITE_ON_BLACK;
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

/* 外部汇编用户程序 */
extern void user_program_asm(void);

/* 用户程序入口 */
void user_program(void) {
    user_program_asm();
}

/* 检查当前特权级 */
static uint32_t get_current_cs(void) {
    uint32_t cs;
    __asm__ volatile("mov %%cs, %0" : "=r"(cs));
    return cs;
}

void kernel_main(void) {
    serial_init();
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - User Mode Demo\n");
    print_string("========================================\n\n");

    /* 初始化分配器 */
    print_string("Step 1: Initialize allocator...\n");
    allocator_init(0x100000, 0x100000);
    print_string("  Done!\n\n");

    /* 初始化用户模式 GDT */
    print_string("Step 2: Initialize user mode GDT...\n");
    usermode_init();
    print_string("  Done!\n\n");

    /* 检查当前特权级 */
    print_string("Step 3: Check current privilege level\n");
    uint32_t cs = get_current_cs();
    print_string("  Current CS: ");
    print_hex(cs);
    print_string("\n");

    int cpl = cs & 0x3;
    print_string("  Current CPL: ");
    serial_putchar('0' + cpl);
    print_string(" (Ring ");
    serial_putchar('0' + cpl);
    print_string(")\n\n");

    /* 切换到用户模式 */
    print_string("Step 4: Switch to user mode...\n");
    print_string("  Allocating user stack...\n");
    print_string("  Jumping to Ring 3...\n\n");

    /* 在屏幕上显示内核信息（第一行） */
    char *video = (char *)VIDEO_MEMORY;
    const char *kernel_msg = "KERNEL: Switched to user mode";
    int i = 0;
    for (; kernel_msg[i]; i++) {
        video[i * 2] = kernel_msg[i];
        video[i * 2 + 1] = WHITE_ON_BLACK;
    }

    /* 在第二行显示分隔线 */
    video[160] = '=';
    video[161] = WHITE_ON_BLACK;
    for (int j = 1; j < 38; j++) {
        video[160 + j * 2] = '=';
        video[160 + j * 2 + 1] = WHITE_ON_BLACK;
    }

    /* 创建并运行用户进程 */
    create_user_process("UserProgram", user_program);

    /* 永远不会到这里 */
    print_string("ERROR: Should not reach here!\n");
    while (1) {
        __asm__ volatile("hlt");
    }
}
