/* keyboard.c - 键盘驱动实现 */

#include "keyboard.h"
#include "io.h"
#include "pic.h"

#define KB_DATA_PORT    0x60    /* 键盘数据端口 */
#define KB_STATUS_PORT  0x64    /* 键盘状态端口 */
#define KB_BUFFER_SIZE  256     /* 缓冲区大小 */

/* 键盘缓冲区 */
static char kb_buffer[KB_BUFFER_SIZE];
static int buffer_head = 0;
static int buffer_tail = 0;

/* 扫描码到 ASCII 的映射表（美国键盘布局） */
static const char scancode_to_ascii[] = {
    0,    0,   '1', '2', '3', '4', '5', '6',    /* 0x00-0x07 */
    '7', '8', '9', '0', '-', '=', '\b', '\t',   /* 0x08-0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',    /* 0x10-0x17 */
    'o', 'p', '[', ']', '\n', 0,   'a', 's',    /* 0x18-0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',    /* 0x20-0x27 */
    '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',   /* 0x28-0x2F */
    'b', 'n', 'm', ',', '.', '/', 0,   '*',     /* 0x30-0x37 */
    0,   ' ', 0,   0,   0,   0,   0,   0,       /* 0x38-0x3F */
    0,   0,   0,   0,   0,   0,   0,   '7',    /* 0x40-0x47 */
    '8', '9', '-', '4', '5', '6', '+', '1',    /* 0x48-0x4F */
    '2', '3', '0', '.', 0,   0,   0,   0,       /* 0x50-0x57 */
};

/* Shift 键状态 */
static int shift_pressed = 0;

/* Shift 映射表 */
static const char shift_scancode_to_ascii[] = {
    0,    0,   '!', '@', '#', '$', '%', '^',    /* 0x00-0x07 */
    '&', '*', '(', ')', '_', '+', '\b', '\t',   /* 0x08-0x0F */
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',    /* 0x10-0x17 */
    'O', 'P', '{', '}', '\n', 0,   'A', 'S',    /* 0x18-0x1F */
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',    /* 0x20-0x27 */
    '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',    /* 0x28-0x2F */
    'B', 'N', 'M', '<', '>', '?', 0,   '*',     /* 0x30-0x37 */
};

/* 初始化键盘 */
void keyboard_init(void) {
    /* 清空缓冲区 */
    buffer_head = 0;
    buffer_tail = 0;

    /* 启用键盘中断 (IRQ1) */
    pic_unmask_irq(1);
}

/* 键盘中断处理 */
void keyboard_handler(void) {
    uint8_t scancode;
    char ascii;

    /* 读取扫描码 */
    scancode = inb(KB_DATA_PORT);

    /* 检查是否是按键释放 (0x80 = 释放标志) */
    if (scancode & 0x80) {
        /* 按键释放 */
        scancode &= 0x7F;

        /* 检查 Shift 释放 */
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 0;
        }
        return;
    }

    /* 检查 Shift 按下 */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        return;
    }

    /* 转换为 ASCII */
    if (shift_pressed && scancode < sizeof(shift_scancode_to_ascii)) {
        ascii = shift_scancode_to_ascii[scancode];
    } else if (scancode < sizeof(scancode_to_ascii)) {
        ascii = scancode_to_ascii[scancode];
    } else {
        ascii = 0;
    }

    /* 存入缓冲区 */
    if (ascii != 0) {
        kb_buffer[buffer_head] = ascii;
        buffer_head = (buffer_head + 1) % KB_BUFFER_SIZE;
    }
}

/* 检查是否有按键 */
int keyboard_has_key(void) {
    return buffer_head != buffer_tail;
}

/* 读取键盘缓冲区 */
char keyboard_getchar(void) {
    char c;

    if (buffer_head == buffer_tail) {
        return 0;   /* 缓冲区为空 */
    }

    c = kb_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KB_BUFFER_SIZE;

    return c;
}
