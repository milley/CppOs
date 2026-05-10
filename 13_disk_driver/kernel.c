/* kernel.c - 操作系统内核 */

#include <stdint.h>
#include <stddef.h>
#include "idt.h"
#include "pic.h"
#include "syscall.h"
#include "usermode.h"
#include "allocator.h"
#include "process.h"
#include "paging.h"
#include "disk.h"
#include "filesystem.h"
#include "keyboard.h"
#include "shell.h"

#define VIDEO_MEMORY 0xB8000
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define CYAN_ON_BLACK  0x0B
#define RED_ON_BLACK   0x0C
#define YELLOW_ON_BLACK 0x0E

static int cursor_col = 0;
static int cursor_row = 0;
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
    if (c == '\n') { cursor_col = 0; cursor_row++; return; }
    video[offset] = c;
    video[offset + 1] = WHITE_ON_BLACK;
    cursor_col++;
    if (cursor_col >= MAX_COLS) { cursor_col = 0; cursor_row++; }
}

void print_string(const char *str) {
    while (*str) { print_char(*str); str++; }
}

void print_hex(uint32_t value) {
    char hex[] = "0123456789ABCDEF";
    char buffer[9];
    buffer[8] = '\0';
    for (int i = 7; i >= 0; i--) { buffer[i] = hex[value & 0xF]; value >>= 4; }
    print_string("0x");
    print_string(buffer);
}

void print_hex64(uint64_t value) {
    char hex[] = "0123456789ABCDEF";
    char buffer[17];
    buffer[16] = '\0';
    for (int i = 15; i >= 0; i--) { buffer[i] = hex[value & 0xF]; value >>= 4; }
    print_string("0x");
    print_string(buffer);
}

void print_int(uint32_t value) {
    char buffer[16];
    int i = 0;
    if (value == 0) { print_string("0"); return; }
    while (value > 0) { buffer[i++] = '0' + (value % 10); value /= 10; }
    while (i > 0) { print_char(buffer[--i]); }
}

void clear_screen(void) {
    char *video = (char *)VIDEO_MEMORY;
    for (int i = 0; i < MAX_COLS * 25 * 2; i += 2) {
        video[i] = ' '; video[i + 1] = WHITE_ON_BLACK;
    }
    cursor_col = 0; cursor_row = 0;
}

void timer_handler(struct interrupt_frame *frame) {
    timer_ticks++;
    char *video = (char *)VIDEO_MEMORY;
    int offset = 2 * 160 + 10 * 2;
    uint32_t secs = timer_ticks / 18;
    video[offset] = '0' + (secs % 10);
    video[offset + 1] = CYAN_ON_BLACK;

    struct process *current = process_get_current();
    if (current != NULL) {
        current->ticks_remaining--;
        if (current->ticks_remaining == 0) { schedule_from_interrupt(frame); }
    }
    pic_send_eoi(0);
}

void keyboard_handler(struct interrupt_frame *frame) {
    keyboard_irq_handler();
    pic_send_eoi(1);
}

void test_process(void) {
    uint32_t pid = sys_getpid();
    char *video = (char *)0xB8000;
    int count = 0;
    while (1) {
        int offset = 18 * 160;
        video[offset] = 'P'; video[offset + 1] = GREEN_ON_BLACK;
        video[offset + 2] = 'I'; video[offset + 3] = GREEN_ON_BLACK;
        video[offset + 4] = 'D'; video[offset + 5] = GREEN_ON_BLACK;
        video[offset + 6] = ':'; video[offset + 7] = GREEN_ON_BLACK;
        video[offset + 8] = '0' + pid; video[offset + 9] = GREEN_ON_BLACK;
        video[offset + 12] = 'C'; video[offset + 13] = GREEN_ON_BLACK;
        video[offset + 14] = '0' + (count % 10); video[offset + 15] = GREEN_ON_BLACK;
        for (volatile int i = 0; i < 100000; i++);
        count++;
    }
}

static uint8_t test_buffer[DISK_SECTOR_SIZE * 2];

void kernel_main(void) {
    clear_screen();

    print_string("=== MyOS Boot ===\n\n");

    print_string("Init allocator...\n");
    allocator_init(0x100000, 0x100000);

    print_string("Init paging...\n");
    paging_init();

    print_string("Init PIC & IDT...\n");
    pic_init();
    idt_init();

    print_string("Init disk...\n");
    disk_init();

    int disk_count = disk_get_count();
    if (disk_count == 0) {
        print_string("No disk found!\n");
        while (1) { __asm__ volatile("hlt"); }
    }

    disk_t *disk = disk_get_primary();
    print_string("Disk: ");
    print_string(disk->model);
    print_string(" (");
    print_int((uint32_t)(disk->sectors / 2048));
    print_string(" MB)\n");

    /* 初始化或格式化文件系统 */
    fs_init(disk);
    if (!fs_is_valid()) {
        print_string("Formatting disk...\n");
        if (fs_format(disk) == 0) {
            print_string("Format complete.\n");
        } else {
            print_string("Format failed!\n");
            while (1) { __asm__ volatile("hlt"); }
        }
    } else {
        print_string("File system loaded.\n");
    }

    /* 启用中断 */
    print_string("Enabling interrupts...\n");
    idt_register_handler(IRQ_TIMER, timer_handler);
    idt_register_handler(IRQ_KEYBOARD, keyboard_handler);
    enable_interrupts();
    pic_unmask_irq(0);
    pic_unmask_irq(1);

    print_string("Boot complete!\n\n");

    /* 启动 Shell */
    shell_init();
    shell_run();

    while (1) { __asm__ volatile("hlt"); }
}
