/* syscall.h - 系统调用头文件 */

#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include "idt.h"

/* 系统调用号 */
#define SYS_EXIT        0   /* 退出进程 */
#define SYS_READ        1   /* 读取 */
#define SYS_WRITE       2   /* 写入 */
#define SYS_GETPID      3   /* 获取进程 ID */
#define SYS_SLEEP       4   /* 睡眠 */
#define SYS_PUTS        5   /* 打印字符串 */
#define SYS_PUTC        6   /* 打印字符 */
#define SYS_GETTICKS    7   /* 获取时钟计数 */
#define SYS_FORK        8   /* 创建子进程 */
#define SYS_EXEC        9   /* 执行程序 */
#define SYS_WAIT        10  /* 等待子进程 */

/* 系统调用宏（用户程序使用） */
#define syscall0(num) ({ \
    uint32_t ret; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num) \
    ); \
    ret; \
})

#define syscall1(num, arg1) ({ \
    uint32_t ret; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(arg1) \
    ); \
    ret; \
})

#define syscall2(num, arg1, arg2) ({ \
    uint32_t ret; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(arg1), "c"(arg2) \
    ); \
    ret; \
})

#define syscall3(num, arg1, arg2, arg3) ({ \
    uint32_t ret; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3) \
    ); \
    ret; \
})

/* 用户程序使用的系统调用封装 */
static inline void sys_exit(int status) {
    syscall1(SYS_EXIT, status);
}

static inline int sys_puts(const char *str) {
    return syscall1(SYS_PUTS, (uint32_t)str);
}

static inline int sys_putc(char c) {
    return syscall1(SYS_PUTC, c);
}

static inline uint32_t sys_getticks(void) {
    return syscall0(SYS_GETTICKS);
}

static inline void sys_sleep(uint32_t ticks) {
    syscall1(SYS_SLEEP, ticks);
}

static inline uint32_t sys_getpid(void) {
    return syscall0(SYS_GETPID);
}

static inline int sys_read(int fd, char *buf, uint32_t count) {
    return syscall3(SYS_READ, (uint32_t)fd, (uint32_t)buf, count);
}

static inline int sys_write(int fd, const char *buf, uint32_t count) {
    return syscall3(SYS_WRITE, (uint32_t)fd, (uint32_t)buf, count);
}

static inline int sys_fork(void) {
    return syscall0(SYS_FORK);
}

static inline int sys_exec(void (*entry)(void)) {
    return syscall1(SYS_EXEC, (uint32_t)entry);
}

static inline int sys_wait(int *status) {
    return syscall1(SYS_WAIT, (uint32_t)status);
}

/* 函数声明 */

/* 初始化系统调用 */
void syscall_init(void);

/* 系统调用处理函数 */
void syscall_handler(struct interrupt_frame *frame);

#endif
