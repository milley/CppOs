/* memory.c - 内存检测实现 */

#include "memory.h"
#include <stdint.h>

#define VIDEO_MEMORY 0xB8000
#define MAX_COLS 80
#define MAX_ROWS 25

/* 内存映射缓冲区 */
#define MAX_ENTRIES 32
static struct memory_map_entry memory_map[MAX_ENTRIES];
static struct memory_info mem_info = {0, 0, 0};

/* 简单的打印函数（避免依赖内核其他部分） */
static void print_char(char c, int col, int row) {
    char *video = (char *)VIDEO_MEMORY;
    int offset = (row * MAX_COLS + col) * 2;
    video[offset] = c;
    video[offset + 1] = 0x0F;
}

static void print_string_at(const char *str, int *col, int row) {
    while (*str) {
        if (*str == '\n') {
            *col = 0;
            return;
        }
        print_char(*str, *col, row);
        (*col)++;
        str++;
    }
}

static void print_hex(uint32_t value, int *col, int row) {
    char hex[] = "0123456789ABCDEF";
    char buffer[9];

    buffer[8] = '\0';
    for (int i = 7; i >= 0; i--) {
        buffer[i] = hex[value & 0xF];
        value >>= 4;
    }

    print_string_at(buffer, col, row);
}

static void print_decimal(uint32_t value, int *col, int row) {
    char buffer[16];
    int i = 0;

    if (value == 0) {
        print_char('0', *col, row);
        (*col)++;
        return;
    }

    while (value > 0) {
        buffer[i++] = '0' + (value % 10);
        value /= 10;
    }

    while (i > 0) {
        print_char(buffer[--i], *col, row);
        (*col)++;
    }
}

/* 使用 BIOS INT 0x15, EAX=0xE820 检测内存 */
/* 注意：这需要在实模式下调用，所以我们在引导程序中完成 */
/* 这里只是解析引导程序传递的数据 */

/* 内存检测 - 从 BIOS 获取的数据解析 */
void memory_detect(void) {
    /* 引导程序会在 0x5000 处存放内存映射信息 */
    /* 格式：条目数(4字节) + 条目数组 */

    uint32_t *entry_count_ptr = (uint32_t *)0x5000;
    mem_info.entry_count = *entry_count_ptr;

    if (mem_info.entry_count > MAX_ENTRIES) {
        mem_info.entry_count = MAX_ENTRIES;
    }

    struct memory_map_entry *entries = (struct memory_map_entry *)0x5004;

    for (uint32_t i = 0; i < mem_info.entry_count; i++) {
        memory_map[i] = entries[i];

        /* 统计可用内存 */
        if (entries[i].type == MEMORY_TYPE_USABLE) {
            mem_info.usable_memory += entries[i].length / 1024;
        }
        mem_info.total_memory += entries[i].length / 1024;
    }
}

/* 打印内存信息 */
void memory_print_info(void) {
    int col = 0;
    int row = 0;

    print_string_at("Memory Map:\n", &col, row);
    row++;

    for (uint32_t i = 0; i < mem_info.entry_count; i++) {
        col = 0;

        /* 基地址 */
        print_string_at("Base: 0x", &col, row);
        print_hex((uint32_t)memory_map[i].base_address, &col, row);
        print_string_at(" ", &col, row);

        /* 长度 */
        print_string_at("Size: ", &col, row);
        print_decimal(memory_map[i].length / 1024, &col, row);
        print_string_at(" KB ", &col, row);

        /* 类型 */
        print_string_at("Type: ", &col, row);
        print_decimal(memory_map[i].type, &col, row);

        row++;
    }

    row++;
    col = 0;
    print_string_at("Total: ", &col, row);
    print_decimal(mem_info.total_memory, &col, row);
    print_string_at(" KB\n", &col, row);
    row++;

    col = 0;
    print_string_at("Usable: ", &col, row);
    print_decimal(mem_info.usable_memory, &col, row);
    print_string_at(" KB\n", &col, row);
}

uint32_t memory_get_total(void) {
    return mem_info.total_memory;
}

uint32_t memory_get_usable(void) {
    return mem_info.usable_memory;
}
