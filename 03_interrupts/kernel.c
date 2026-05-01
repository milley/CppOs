/* kernel.c - 主内核（简化版） */

#include "idt.h"
#include "pic.h"
#include "io.h"

#define VIDEO_MEMORY 0xB8000
#define MAX_COLS 80
#define MAX_ROWS 25

#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define CYAN_ON_BLACK  0x0B

static int cursor_col = 0;
static int cursor_row = 0;

void print_char(char c, int col, int row, char attr) {
    char *video = (char *)VIDEO_MEMORY;
    int offset;

    if (col >= 0 && row >= 0) {
        offset = (row * MAX_COLS + col) * 2;
    } else {
        offset = (cursor_row * MAX_COLS + cursor_col) * 2;
    }

    if (c == '\n') {
        cursor_row++;
        cursor_col = 0;
        return;
    }

    video[offset] = c;
    video[offset + 1] = attr;

    cursor_col++;
    if (cursor_col >= MAX_COLS) {
        cursor_col = 0;
        cursor_row++;
    }
}

void print_string(const char *str, char attr) {
    int i = 0;
    while (str[i] != '\0') {
        print_char(str[i], -1, -1, attr);
        i++;
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

/* 测试除零异常 */
void test_division_by_zero(void) {
    print_string("Testing division by zero...\n", WHITE_ON_BLACK);
    int a = 1;
    int b = 0;
    int c = a / b;  /* 这会触发除零异常 */
    print_string("Result: ", WHITE_ON_BLACK);
}

void kernel_main(void) {
    clear_screen();

    print_string("========================================\n", CYAN_ON_BLACK);
    print_string("   MyOS - Interrupt Handling Demo\n", GREEN_ON_BLACK);
    print_string("========================================\n", CYAN_ON_BLACK);
    print_string("\n", WHITE_ON_BLACK);

    print_string("Initializing IDT... ", WHITE_ON_BLACK);
    idt_init();
    print_string("OK\n", GREEN_ON_BLACK);

    print_string("Initializing PIC... ", WHITE_ON_BLACK);
    pic_init();
    print_string("OK\n", GREEN_ON_BLACK);

    print_string("\nEnabling interrupts...\n", WHITE_ON_BLACK);
    __asm__ volatile("sti");

    print_string("\nSystem ready!\n", GREEN_ON_BLACK);
    print_string("Press any key to test exception...\n", WHITE_ON_BLACK);

    /* 等待按键 */
    __asm__ volatile("hlt");

    /* 测试异常 */
    test_division_by_zero();

    while (1) {
        __asm__ volatile("hlt");
    }
}