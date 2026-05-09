/* syscall.c - 系统调用实现 */

#include "syscall.h"
#include "idt.h"

/* 当前进程 ID（简化版） */
static uint32_t current_pid = 1;

/* 时钟计数（外部定义） */
extern uint32_t timer_ticks;

/* 系统调用：退出进程 */
static uint32_t sys_exit_handler(int status) {
    /* 简化版：显示退出信息并挂起 */
    char *video = (char *)0xB8000;
    const char *msg = "Process exited with code ";
    int offset = 20 * 160;  /* 第 21 行 */

    for (int i = 0; msg[i]; i++) {
        video[offset + i * 2] = msg[i];
        video[offset + i * 2 + 1] = 0x0C;
    }

    /* 显示退出码 */
    char code[12];
    int i = 0;
    if (status == 0) {
        code[i++] = '0';
    } else {
        int s = status;
        while (s > 0) {
            code[i++] = '0' + (s % 10);
            s /= 10;
        }
    }
    int pos = 28;
    while (i > 0) {
        video[offset + pos * 2] = code[--i];
        video[offset + pos * 2 + 1] = 0x0C;
        pos++;
    }

    /* 挂起 */
    while (1) {
        __asm__ volatile("hlt");
    }

    return 0;
}

/* 系统调用：打印字符串 */
static uint32_t sys_puts_handler(const char *str) {
    char *video = (char *)0xB8000;
    static int row = 10;

    /* 找到空行 */
    int offset = row * 160;

    for (int i = 0; str[i] && i < 78; i++) {
        video[offset + i * 2] = str[i];
        video[offset + i * 2 + 1] = 0x0A;  /* 绿色 */
    }

    row++;
    if (row >= 20) row = 10;

    return 0;
}

/* 系统调用：打印字符 */
static uint32_t sys_putc_handler(char c) {
    static int col = 0;
    static int row = 12;

    char *video = (char *)0xB8000;
    int offset = row * 160 + col * 2;

    if (c == '\n') {
        col = 0;
        row++;
        if (row >= 20) row = 12;
        return 0;
    }

    video[offset] = c;
    video[offset + 1] = 0x0B;  /* 青色 */

    col++;
    if (col >= 80) {
        col = 0;
        row++;
        if (row >= 20) row = 12;
    }

    return 0;
}

/* 端口 I/O */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* 系统调用：读取（从键盘缓冲区读取字符） */
static uint32_t sys_read_handler(uint32_t fd, char *buf, uint32_t count) {
    (void)fd;  /* 简化版：忽略文件描述符 */

    if (buf == (char *)0 || count == 0) {
        return 0;
    }

    /* 等待键盘数据可用 */
    while ((inb(0x64) & 0x01) == 0) {
        __asm__ volatile("hlt");
    }

    /* 读取扫描码 */
    uint8_t scancode = inb(0x60);

    /* 简单的扫描码到 ASCII 转换（仅小写字母） */
    char c = 0;
    if (scancode >= 0x1E && scancode <= 0x26) {
        /* A-Z 键 */
        c = 'a' + (scancode - 0x1E);
    } else if (scancode >= 0x02 && scancode <= 0x0B) {
        /* 数字键 1-9, 0 */
        c = (scancode == 0x0B) ? '0' : '1' + (scancode - 0x02);
    } else if (scancode == 0x1C) {
        c = '\n';
    } else if (scancode == 0x39) {
        c = ' ';
    }

    if (c != 0) {
        buf[0] = c;
        return 1;
    }

    return 0;
}

/* 系统调用：写入（输出到屏幕） */
static uint32_t sys_write_handler(uint32_t fd, const char *buf, uint32_t count) {
    (void)fd;  /* 简化版：忽略文件描述符，总是写到屏幕 */

    if (buf == (const char *)0 || count == 0) {
        return 0;
    }

    char *video = (char *)0xB8000;
    static int row = 10;
    static int col = 0;

    uint32_t written = 0;

    for (uint32_t i = 0; i < count; i++) {
        char c = buf[i];

        if (c == '\n') {
            col = 0;
            row++;
            if (row >= 20) row = 10;
            written++;
            continue;
        }

        int offset = row * 160 + col * 2;
        video[offset] = c;
        video[offset + 1] = 0x0A;  /* 绿色 */

        col++;
        if (col >= 80) {
            col = 0;
            row++;
            if (row >= 20) row = 10;
        }

        written++;
    }

    return written;
}

/* 系统调用：获取时钟计数 */
static uint32_t sys_getticks_handler(void) {
    return timer_ticks;
}

/* 系统调用：睡眠 */
static uint32_t sys_sleep_handler(uint32_t ticks) {
    uint32_t target = timer_ticks + ticks;
    while (timer_ticks < target) {
        __asm__ volatile("hlt");
    }
    return 0;
}

/* 系统调用：获取进程 ID */
static uint32_t sys_getpid_handler(void) {
    return current_pid;
}

/* 系统调用表 */
typedef uint32_t (*syscall_func_t)(uint32_t, uint32_t, uint32_t);

static syscall_func_t syscall_table[] = {
    [SYS_EXIT]     = (syscall_func_t)sys_exit_handler,
    [SYS_READ]     = (syscall_func_t)sys_read_handler,
    [SYS_WRITE]    = (syscall_func_t)sys_write_handler,
    [SYS_GETPID]   = (syscall_func_t)sys_getpid_handler,
    [SYS_SLEEP]    = (syscall_func_t)sys_sleep_handler,
    [SYS_PUTS]     = (syscall_func_t)sys_puts_handler,
    [SYS_PUTC]     = (syscall_func_t)sys_putc_handler,
    [SYS_GETTICKS] = (syscall_func_t)sys_getticks_handler,
};

#define SYSCALL_COUNT (sizeof(syscall_table) / sizeof(syscall_table[0]))

/* 系统调用处理函数 */
void syscall_handler(struct interrupt_frame *frame) {
    /* eax = 系统调用号, ebx, ecx, edx = 参数 */
    uint32_t syscall_num = frame->eax;
    uint32_t arg1 = frame->ebx;
    uint32_t arg2 = frame->ecx;
    uint32_t arg3 = frame->edx;

    uint32_t ret = -1;  /* 默认返回 -1 表示错误 */

    if (syscall_num < SYSCALL_COUNT && syscall_table[syscall_num]) {
        ret = syscall_table[syscall_num](arg1, arg2, arg3);
    }

    /* 返回值放在 eax */
    frame->eax = ret;
}

/* 初始化系统调用 */
void syscall_init(void) {
    /* 注册系统调用处理函数到 IDT 0x80 */
    idt_register_handler(0x80, syscall_handler);
}
