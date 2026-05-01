/* kernel.c - 主内核（轮询方式测试） */

#include "idt.h"
#include "pic.h"
#include "keyboard.h"
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

    /* 处理退格键 */
    if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            offset = (cursor_row * MAX_COLS + cursor_col) * 2;
            video[offset] = ' ';
            video[offset + 1] = attr;
        }
        return;
    }

    /* 处理回车键 */
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

/* 扫描码转 ASCII（简化版） */
/* 特殊扫描码 */
#define SCAN_ESC       0x01
#define SCAN_BACKSPACE 0x0E
#define SCAN_ENTER     0x1C

/* 特殊 ASCII 码 */
#define ASCII_ESC      0x1B

static const char scancode_table[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', 0,
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0, 'a', 's',
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\', 'z', 'x', 'c', 'v',
    'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

/* 轮询方式读取键盘 */
char poll_keyboard(void) {
    uint8_t status, scancode;

    /* 检查键盘状态 */
    status = inb(0x64);
    if (!(status & 0x01)) {
        return 0;  /* 无数据 */
    }

    /* 读取扫描码 */
    scancode = inb(0x60);

    /* 只处理按下事件 */
    if (scancode & 0x80) {
        return 0;  /* 释放事件 */
    }

    /* 特殊处理 ESC 键 */
    if (scancode == SCAN_ESC) {
        return ASCII_ESC;
    }

    /* 转换为 ASCII */
    if (scancode < sizeof(scancode_table)) {
        return scancode_table[scancode];
    }

    return 0;
}

void kernel_main(void) {
    char c;

    clear_screen();

    print_string("========================================\n", CYAN_ON_BLACK);
    print_string("   MyOS - Keyboard Polling Test\n", GREEN_ON_BLACK);
    print_string("========================================\n", CYAN_ON_BLACK);
    print_string("\n", WHITE_ON_BLACK);

    print_string("Initializing IDT... ", WHITE_ON_BLACK);
    idt_init();
    print_string("OK\n", GREEN_ON_BLACK);

    print_string("Initializing PIC... ", WHITE_ON_BLACK);
    pic_init();
    print_string("OK\n", GREEN_ON_BLACK);

    print_string("\nReady! Type something:\n", GREEN_ON_BLACK);
    print_string("> ", WHITE_ON_BLACK);

    /* 不启用中断，使用轮询方式 */
    while (1) {
        c = poll_keyboard();
        if (c != 0) {
            /* ESC 键退出 */
            if (c == ASCII_ESC) {
                print_string("\n\nGoodbye!\n", CYAN_ON_BLACK);
                break;
            }
            print_char(c, -1, -1, WHITE_ON_BLACK);
        }
    }

    while (1) {
        __asm__ volatile("hlt");
    }
}