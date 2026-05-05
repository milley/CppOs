/* kernel.c - 主内核（简化版） */

#include "memory.h"
#include <stdint.h>

#define VIDEO_MEMORY 0xB8000
#define MAX_COLS 80
#define MAX_ROWS 25

#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define CYAN_ON_BLACK  0x0B

static int cursor_col = 0;
static int cursor_row = 0;

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

void kernel_main(void) {
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - Memory Detection Demo\n");
    print_string("========================================\n\n");

    print_string("Detecting memory...\n\n");

    /* 读取内存映射信息 */
    /* 地址 0x5000 存放条目数 */
    uint32_t *entry_count_ptr = (uint32_t *)0x5000;
    uint32_t entry_count = *entry_count_ptr;

    print_string("Memory entries found: ");
    print_decimal(entry_count);
    print_string("\n\n");

    /* 内存映射条目从 0x5004 开始 */
    /* 每个条目 24 字节 */
    uint8_t *entries = (uint8_t *)0x5004;

    uint32_t total_kb = 0;
    uint32_t usable_kb = 0;

    for (uint32_t i = 0; i < entry_count && i < 10; i++) {
        /* 每个条目 24 字节 */
        uint8_t *entry = entries + (i * 24);

        /* 解析基地址 (8字节) */
        uint32_t base_low = *(uint32_t *)(entry + 0);
        uint32_t base_high = *(uint32_t *)(entry + 4);

        /* 解析长度 (8字节) */
        uint32_t len_low = *(uint32_t *)(entry + 8);
        uint32_t len_high = *(uint32_t *)(entry + 12);

        /* 解析类型 (4字节) */
        uint32_t type = *(uint32_t *)(entry + 16);

        /* 打印条目信息 */
        print_string("[");
        print_decimal(i);
        print_string("] ");

        print_string("Base: ");
        print_hex(base_low);

        print_string(" Size: ");
        print_decimal(len_low / 1024);
        print_string(" KB");

        print_string(" Type: ");
        print_decimal(type);

        if (type == 1) {
            print_string(" (Usable)");
            usable_kb += len_low / 1024;
        } else {
            print_string(" (Reserved)");
        }

        print_string("\n");

        total_kb += len_low / 1024;
    }

    print_string("\n----------------------------------------\n");
    print_string("Summary:\n");
    print_string("  Total Memory:  ");
    print_decimal(total_kb);
    print_string(" KB\n");

    print_string("  Usable Memory: ");
    print_decimal(usable_kb);
    print_string(" KB\n");

    print_string("\nMemory detection complete!\n");

    while (1) {
        __asm__ volatile("hlt");
    }
}
