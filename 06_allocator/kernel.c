/* kernel.c - 主内核 */

#include <stdint.h>
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

/* 串口输出（用于调试） */
static void serial_init(void) {
    outb(SERIAL_PORT + 1, 0x00);    /* 禁用中断 */
    outb(SERIAL_PORT + 3, 0x80);    /* 启用 DLAB */
    outb(SERIAL_PORT + 0, 0x03);    /* 波特率 38400 */
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);    /* 8 bits, no parity, one stop bit */
    outb(SERIAL_PORT + 2, 0xC7);    /* FIFO */
    outb(SERIAL_PORT + 4, 0x0B);    /* IRQs, RTS/DSR set */
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
    serial_print(str);  /* 同时输出到串口 */
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
    serial_init();  /* 初始化串口 */
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - Memory Allocator Demo\n");
    print_string("========================================\n\n");

    /* 初始化分配器：堆从 1MB 开始，大小 1MB */
    print_string("Initializing allocator...\n");
    allocator_init(0x100000, 0x100000);  /* 1MB 堆 */
    print_string("Done!\n\n");

    print_memory_status();

    /* ========== 测试 1: 基本分配和释放 ========== */
    print_string("Test 1: Basic allocation\n");
    print_string("----------------------------------------\n");

    void *ptr1 = kmalloc(100);
    print_string("  kmalloc(100) = ");
    print_hex((uint32_t)ptr1);
    print_string("\n");

    void *ptr2 = kmalloc(500);
    print_string("  kmalloc(500) = ");
    print_hex((uint32_t)ptr2);
    print_string("\n");

    void *ptr3 = kmalloc(200);
    print_string("  kmalloc(200) = ");
    print_hex((uint32_t)ptr3);
    print_string("\n");

    /* 验证地址不同 */
    if (ptr1 != ptr2 && ptr2 != ptr3 && ptr1 != ptr3) {
        print_string("  [PASS] All addresses are different\n");
    } else {
        print_string("  [FAIL] Addresses conflict!\n");
    }

    /* 验证地址在堆范围内 */
    if ((uint32_t)ptr1 >= 0x100000 && (uint32_t)ptr1 < 0x200000) {
        print_string("  [PASS] ptr1 in heap range\n");
    } else {
        print_string("  [FAIL] ptr1 out of range!\n");
    }

    print_string("\n");

    /* ========== 测试 2: 内存统计 ========== */
    print_string("Test 2: Memory statistics\n");
    print_string("----------------------------------------\n");

    uint32_t used_before = get_used_memory();
    print_string("  Used before free: ");
    print_decimal(used_before);
    print_string(" bytes\n");

    kfree(ptr2);
    uint32_t used_after = get_used_memory();
    print_string("  Used after free ptr2: ");
    print_decimal(used_after);
    print_string(" bytes\n");

    if (used_after < used_before) {
        print_string("  [PASS] Used memory decreased after free\n");
    } else {
        print_string("  [FAIL] Used memory did not decrease!\n");
    }

    print_string("\n");

    /* ========== 测试 3: 写入验证 ========== */
    print_string("Test 3: Write and read test\n");
    print_string("----------------------------------------\n");

    /* 写入数据 */
    char *data = (char *)ptr1;
    data[0] = 'H';
    data[1] = 'E';
    data[2] = 'L';
    data[3] = 'L';
    data[4] = 'O';
    data[5] = '\0';

    /* 读取并验证 */
    if (data[0] == 'H' && data[1] == 'E' && data[2] == 'L') {
        print_string("  [PASS] Write/read works: ");
        print_string(data);
        print_string("\n");
    } else {
        print_string("  [FAIL] Write/read failed!\n");
    }

    print_string("\n");

    /* ========== 测试 4: 重用已释放内存 ========== */
    print_string("Test 4: Reuse freed memory\n");
    print_string("----------------------------------------\n");

    void *ptr4 = kmalloc(300);
    print_string("  kmalloc(300) = ");
    print_hex((uint32_t)ptr4);
    print_string("\n");

    /* ptr4 应该重用 ptr2 释放的空间（500 > 300） */
    if ((uint32_t)ptr4 == (uint32_t)ptr2) {
        print_string("  [PASS] Reused freed memory\n");
    } else {
        print_string("  [INFO] Used different location (may split)\n");
    }

    print_string("\n");

    /* ========== 测试 5: 页分配 ========== */
    print_string("Test 5: Page allocation\n");
    print_string("----------------------------------------\n");

    void *page1 = alloc_page();
    print_string("  alloc_page() = ");
    print_hex((uint32_t)page1);
    print_string("\n");

    void *page2 = alloc_page();
    print_string("  alloc_page() = ");
    print_hex((uint32_t)page2);
    print_string("\n");

    /* 两个页应该不同 */
    if (page1 != page2) {
        print_string("  [PASS] Different pages allocated\n");
    } else {
        print_string("  [FAIL] Same page returned!\n");
    }

    print_string("\n");

    /* ========== 测试 6: 全部释放 ========== */
    print_string("Test 6: Free all and verify\n");
    print_string("----------------------------------------\n");

    kfree(ptr1);
    kfree(ptr3);
    kfree(ptr4);
    free_page(page1);
    free_page(page2);

    uint32_t final_free = get_free_memory();
    uint32_t final_used = get_used_memory();

    print_string("  Final used: ");
    print_decimal(final_used);
    print_string(" bytes\n");

    print_string("  Final free: ");
    print_decimal(final_free);
    print_string(" bytes\n");

    if (final_used == 0) {
        print_string("  [PASS] All memory freed\n");
    } else {
        print_string("  [WARN] Some memory still used (fragmentation)\n");
    }

    print_string("\n========================================\n");
    print_string("   All tests completed!\n");
    print_string("========================================\n");

    while (1) {
        __asm__ volatile("hlt");
    }
}
