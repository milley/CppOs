/* kernel.c - 测试分页进程管理 */

#include <stdint.h>
#include <stddef.h>
#include "idt.h"
#include "pic.h"
#include "syscall.h"
#include "usermode.h"
#include "allocator.h"
#include "process.h"
#include "paging.h"

#define VIDEO_MEMORY 0xB8000
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define CYAN_ON_BLACK  0x0B
#define RED_ON_BLACK   0x0C

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
        if (current->ticks_remaining == 0) {
            schedule_from_interrupt(frame);
        }
    }
    pic_send_eoi(0);
}

void keyboard_handler(struct interrupt_frame *frame) {
    uint8_t scancode = inb(0x60);
    (void)scancode;
    pic_send_eoi(1);
}

void test_process_1(void) {
    uint32_t pid = sys_getpid();
    char *video = (char *)0xB8000;
    int count = 0;
    while (1) {
        int offset = 5 * 160;
        video[offset] = 'P'; video[offset + 1] = RED_ON_BLACK;
        video[offset + 2] = '1'; video[offset + 3] = RED_ON_BLACK;
        video[offset + 4] = ':'; video[offset + 5] = RED_ON_BLACK;
        video[offset + 6] = '0' + pid; video[offset + 7] = RED_ON_BLACK;
        video[offset + 10] = 'C'; video[offset + 11] = RED_ON_BLACK;
        video[offset + 12] = '0' + (count % 10); video[offset + 13] = RED_ON_BLACK;
        for (volatile int i = 0; i < 100000; i++);
        count++;
    }
}

void test_process_2(void) {
    uint32_t pid = sys_getpid();
    char *video = (char *)0xB8000;
    int count = 0;
    while (1) {
        int offset = 6 * 160;
        video[offset] = 'P'; video[offset + 1] = GREEN_ON_BLACK;
        video[offset + 2] = '2'; video[offset + 3] = GREEN_ON_BLACK;
        video[offset + 4] = ':'; video[offset + 5] = GREEN_ON_BLACK;
        video[offset + 6] = '0' + pid; video[offset + 7] = GREEN_ON_BLACK;
        video[offset + 10] = 'C'; video[offset + 11] = GREEN_ON_BLACK;
        video[offset + 12] = '0' + (count % 10); video[offset + 13] = GREEN_ON_BLACK;
        for (volatile int i = 0; i < 100000; i++);
        count++;
    }
}

void test_process_3(void) {
    uint32_t pid = sys_getpid();
    char *video = (char *)0xB8000;
    int count = 0;
    while (1) {
        int offset = 7 * 160;
        video[offset] = 'P'; video[offset + 1] = CYAN_ON_BLACK;
        video[offset + 2] = '3'; video[offset + 3] = CYAN_ON_BLACK;
        video[offset + 4] = ':'; video[offset + 5] = CYAN_ON_BLACK;
        video[offset + 6] = '0' + pid; video[offset + 7] = CYAN_ON_BLACK;
        video[offset + 10] = 'C'; video[offset + 11] = CYAN_ON_BLACK;
        video[offset + 12] = '0' + (count % 10); video[offset + 13] = CYAN_ON_BLACK;
        for (volatile int i = 0; i < 100000; i++);
        count++;
    }
}

void kernel_main(void) {
    clear_screen();

    print_string("========================================\n");
    print_string("   MyOS - Paging + Process Demo\n");
    print_string("========================================\n\n");

    print_string("Step 1: Initialize allocator...\n");
    allocator_init(0x100000, 0x100000);
    print_string("  Done!\n\n");

    print_string("Step 2: Initialize paging...\n");
    paging_init();
    print_string("  Kernel page directory created\n");
    print_string("  Free pages: ");
    print_hex(get_free_physical_pages() * 4);
    print_string(" KB\n\n");

    print_string("Step 3: Initialize PIC...\n");
    pic_init();
    print_string("  Done!\n\n");

    print_string("Step 4: Initialize IDT...\n");
    idt_init();
    print_string("  Done!\n\n");

    print_string("Step 5: Initialize user mode...\n");
    usermode_init();
    print_string("  Done!\n\n");

    print_string("Step 6: Initialize syscalls...\n");
    syscall_init();
    print_string("  Done!\n\n");

    print_string("Step 7: Initialize process manager...\n");
    scheduler_init();
    print_string("  Done!\n\n");

    print_string("Step 8: Register interrupt handlers...\n");
    idt_register_handler(IRQ_TIMER, timer_handler);
    idt_register_handler(IRQ_KEYBOARD, keyboard_handler);
    print_string("  Done!\n\n");

    print_string("Step 9: Enable interrupts...\n");
    enable_interrupts();
    pic_unmask_irq(0);
    pic_unmask_irq(1);
    print_string("  Done!\n\n");

    char *video = (char *)VIDEO_MEMORY;
    int offset = 2 * 160;
    video[offset] = 'T'; video[offset + 1] = CYAN_ON_BLACK;
    video[offset + 2] = 'i'; video[offset + 3] = CYAN_ON_BLACK;
    video[offset + 4] = 'c'; video[offset + 5] = CYAN_ON_BLACK;
    video[offset + 6] = 'k'; video[offset + 7] = CYAN_ON_BLACK;
    video[offset + 8] = 's'; video[offset + 9] = CYAN_ON_BLACK;

    print_string("========================================\n");
    print_string("   Creating processes (each with own address space)\n");
    print_string("========================================\n\n");

    struct process *proc1 = process_create(test_process_1, PRIORITY_NORMAL);
    struct process *proc2 = process_create(test_process_2, PRIORITY_NORMAL);
    struct process *proc3 = process_create(test_process_3, PRIORITY_NORMAL);

    if (proc1) print_string("  Process 1 created (PID: 1)\n");
    if (proc2) print_string("  Process 2 created (PID: 2)\n");
    if (proc3) print_string("  Process 3 created (PID: 3)\n");

    print_string("\n  Free pages remaining: ");
    print_hex(get_free_physical_pages() * 4);
    print_string(" KB\n\n");

    print_string("========================================\n");
    print_string("   Starting scheduler...\n");
    print_string("========================================\n\n");

    if (proc1) {
        process_set_current(proc1);
        switch_to_first(proc1);
    }

    while (1) { __asm__ volatile("hlt"); }
}
