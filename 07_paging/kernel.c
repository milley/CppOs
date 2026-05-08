/* kernel.c - 测试分页机制 */

#include <stdint.h>
#include "paging.h"
#include "allocator.h"

#define VIDEO_MEMORY 0xB8000
#define SERIAL_PORT  0x3F8
#define MAX_COLS 80
#define MAX_ROWS 25

#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A

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

void kernel_main(void) {
    serial_init();
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - Paging Mechanism Demo\n");
    print_string("========================================\n\n");

    /* 先初始化分配器 */
    print_string("Step 1: Initialize allocator...\n");
    allocator_init(0x100000, 0x100000);
    print_string("  Done!\n\n");

    /* 初始化分页 */
    print_string("Step 2: Initialize paging...\n");
    print_string("  Enabling paging...\n");

    paging_init();

    print_string("  Paging enabled!\n");
    print_string("  Page directory at: ");
    print_hex(get_page_directory());
    print_string("\n\n");

    /* ========== 测试 1: 恒等映射验证 ========== */
    print_string("Test 1: Identity mapping verification\n");
    print_string("----------------------------------------\n");

    /* 测试 VGA 内存是否仍然可用 */
    print_string("  Writing to VGA (0xB8000)...\n");
    char *vga = (char *)0xB8000;
    vga[0] = 'V';
    vga[1] = GREEN_ON_BLACK;
    vga[2] = 'G';
    vga[3] = GREEN_ON_BLACK;
    vga[4] = 'A';
    vga[5] = GREEN_ON_BLACK;

    print_string("  [PASS] VGA still works after paging\n\n");

    /* ========== 测试 2: 地址转换 ========== */
    print_string("Test 2: Address translation\n");
    print_string("----------------------------------------\n");

    uint32_t test_virt = 0x00100000;  /* 1MB */
    uint32_t test_phys = get_physical_address(test_virt);

    print_string("  Virtual:  ");
    print_hex(test_virt);
    print_string("\n");

    print_string("  Physical: ");
    print_hex(test_phys);
    print_string("\n");

    if (test_phys == test_virt) {
        print_string("  [PASS] Identity mapping works\n");
    } else {
        print_string("  [FAIL] Identity mapping broken!\n");
    }

    print_string("\n");

    /* ========== 测试 3: 创建新映射 ========== */
    print_string("Test 3: Create new mapping\n");
    print_string("----------------------------------------\n");

    /* 映射虚拟地址 0xC0000000 到物理地址 0x00200000 */
    uint32_t virt_addr = 0xC0000000;  /* 3GB - 常用于内核高地址映射 */
    uint32_t phys_addr = 0x00200000;  /* 2MB 物理地址 */

    print_string("  Mapping ");
    print_hex(virt_addr);
    print_string(" -> ");
    print_hex(phys_addr);
    print_string("\n");

    map_page(virt_addr, phys_addr, PAGE_PRESENT | PAGE_WRITABLE);

    /* 验证映射 */
    uint32_t translated = get_physical_address(virt_addr);
    print_string("  Translated: ");
    print_hex(translated);
    print_string("\n");

    if (translated == phys_addr) {
        print_string("  [PASS] New mapping created successfully\n");
    } else {
        print_string("  [FAIL] Mapping failed!\n");
    }

    print_string("\n");

    /* ========== 测试 4: 写入测试 ========== */
    print_string("Test 4: Write to mapped memory\n");
    print_string("----------------------------------------\n");

    /* 使用新映射的地址写入数据 */
    uint32_t *test_ptr = (uint32_t *)virt_addr;
    *test_ptr = 0xDEADBEEF;

    /* 通过物理地址读取验证 */
    uint32_t *phys_ptr = (uint32_t *)phys_addr;
    if (*phys_ptr == 0xDEADBEEF) {
        print_string("  [PASS] Write through virtual address works\n");
        print_string("  Value: 0xDEADBEEF\n");
    } else {
        print_string("  [FAIL] Write failed!\n");
    }

    print_string("\n");

    /* ========== 测试 5: 取消映射 ========== */
    print_string("Test 5: Unmap page\n");
    print_string("----------------------------------------\n");

    unmap_page(virt_addr);
    uint32_t after_unmap = get_physical_address(virt_addr);

    print_string("  After unmap: ");
    print_hex(after_unmap);
    print_string("\n");

    if (after_unmap == 0) {
        print_string("  [PASS] Page unmapped successfully\n");
    } else {
        print_string("  [FAIL] Unmap failed!\n");
    }

    print_string("\n========================================\n");
    print_string("   All paging tests completed!\n");
    print_string("========================================\n");

    while (1) {
        __asm__ volatile("hlt");
    }
}
