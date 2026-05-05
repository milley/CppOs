/* kernel.c - 主内核 */

#include <stdint.h>
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

void print_memory_status(void) {
    print_string("Memory Status:\n");
    print_string("  Total: ");
    print_decimal(get_total_memory());
    print_string(" bytes\n");

    print_string("  Used:  ");
    print_decimal(get_used_memory());
    print_string(" bytes\n");

    print_string("  Free:  ");
    print_decimal(get_free_memory());
    print_string(" bytes\n\n");
}

void kernel_main(void) {
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - Memory Allocator Demo\n");
    print_string("========================================\n\n");

    /* 初始化分配器：堆从 1MB 开始，大小 1MB */
    print_string("Initializing allocator...\n");
    allocator_init(0x100000, 0x100000);  /* 1MB 堆 */
    print_string("Done!\n\n");

    print_memory_status();

    /* 测试 1: 分配内存 */
    print_string("Test 1: Allocate 100 bytes\n");
    void *ptr1 = kmalloc(100);
    print_string("  ptr1 = ");
    print_hex((uint32_t)ptr1);
    print_string("\n");
    print_memory_status();

    /* 测试 2: 分配更多内存 */
    print_string("Test 2: Allocate 500 bytes\n");
    void *ptr2 = kmalloc(500);
    print_string("  ptr2 = ");
    print_hex((uint32_t)ptr2);
    print_string("\n");
    print_memory_status();

    /* 测试 3: 分配页 */
    print_string("Test 3: Allocate one page (4096 bytes)\n");
    void *page = alloc_page();
    print_string("  page = ");
    print_hex((uint32_t)page);
    print_string("\n");
    print_memory_status();

    /* 测试 4: 释放内存 */
    print_string("Test 4: Free ptr1\n");
    kfree(ptr1);
    print_memory_status();

    /* 测试 5: 释放后再分配 */
    print_string("Test 5: Allocate 50 bytes (should reuse freed space)\n");
    void *ptr3 = kmalloc(50);
    print_string("  ptr3 = ");
    print_hex((uint32_t)ptr3);
    print_string("\n");
    print_memory_status();

    /* 测试 6: 释放所有 */
    print_string("Test 6: Free all allocations\n");
    kfree(ptr2);
    kfree(page);
    kfree(ptr3);
    print_memory_status();

    print_string("========================================\n");
    print_string("   All tests completed!\n");
    print_string("========================================\n");

    while (1) {
        __asm__ volatile("hlt");
    }
}
