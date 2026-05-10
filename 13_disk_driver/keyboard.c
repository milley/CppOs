/* keyboard.c - 键盘驱动实现 */

#include "keyboard.h"
#include "string.h"

/* 键盘端口 */
#define KEYBOARD_DATA    0x60
#define KEYBOARD_STATUS  0x64

/* 缓冲区大小 */
#define BUFFER_SIZE 256

/* 循环缓冲区 */
static char key_buffer[BUFFER_SIZE];
static int read_pos = 0;
static int write_pos = 0;
static int buffer_count = 0;

/* Shift 键状态 */
static int shift_pressed = 0;
static int caps_lock = 0;

/* 扫描码到 ASCII 映射表 (小写) */
static const char scancode_table[128] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t','q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n', 0,
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,  '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ',
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0
};

/* Shift 键按下时的映射表 (大写和符号) */
static const char shift_table[128] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n', 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ',
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0
};

/* 端口 I/O */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* 将字符放入缓冲区 */
static void buffer_put(char c) {
    if (buffer_count < BUFFER_SIZE) {
        key_buffer[write_pos] = c;
        write_pos = (write_pos + 1) % BUFFER_SIZE;
        buffer_count++;
    }
}

/* 键盘中断处理函数 */
void keyboard_irq_handler(void) {
    uint8_t scancode = inb(KEYBOARD_DATA);

    /* 按键释放 (高位置 1) */
    int released = scancode & 0x80;
    scancode &= 0x7F;

    /* 处理修饰键 */
    if (scancode == 0x2A || scancode == 0x36) {  /* 左/右 Shift */
        shift_pressed = released ? 0 : 1;
        return;
    }

    if (scancode == 0x3A && !released) {  /* Caps Lock */
        caps_lock = !caps_lock;
        return;
    }

    /* 只处理按下事件 */
    if (released) {
        return;
    }

    /* 获取 ASCII 字符 */
    char c = shift_pressed ? shift_table[scancode] : scancode_table[scancode];

    if (c == 0) {
        return;  /* 忽略无法识别的键 */
    }

    /* 处理大小写 */
    if (caps_lock && c >= 'a' && c <= 'z') {
        c -= 32;
    } else if (caps_lock && c >= 'A' && c <= 'Z' && !shift_pressed) {
        /* Caps Lock 开启但没按 Shift，保持大写 */
    }

    /* 放入缓冲区 */
    buffer_put(c);
}

/* 初始化键盘 */
void keyboard_init(void) {
    read_pos = 0;
    write_pos = 0;
    buffer_count = 0;
    shift_pressed = 0;
    caps_lock = 0;
}

/* 检查是否有按键 */
int keyboard_has_char(void) {
    return buffer_count > 0;
}

/* 读取一个字符 (阻塞) */
char keyboard_getchar(void) {
    while (buffer_count == 0) {
        __asm__ volatile("hlt");
    }

    char c = key_buffer[read_pos];
    read_pos = (read_pos + 1) % BUFFER_SIZE;
    buffer_count--;
    return c;
}

/* 读取一行 */
int keyboard_getline(char *buffer, int max_len) {
    int i = 0;
    char c;

    while (i < max_len - 1) {
        c = keyboard_getchar();

        if (c == '\n') {
            buffer[i] = '\0';
            return i;
        } else if (c == '\b') {
            if (i > 0) {
                i--;
            }
        } else {
            buffer[i++] = c;
        }
    }

    buffer[i] = '\0';
    return i;
}
