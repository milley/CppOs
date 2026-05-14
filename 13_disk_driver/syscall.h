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

/* 文件系统调用 */
#define SYS_OPEN        11  /* 打开文件 */
#define SYS_CLOSE       12  /* 关闭文件 */
#define SYS_FREAD       13  /* 读取文件 */
#define SYS_FWRITE      14  /* 写入文件 */
#define SYS_FCREATE     15  /* 创建文件 */
#define SYS_FDELETE     16  /* 删除文件 */
#define SYS_FSIZE       17  /* 获取文件大小 */
#define SYS_FLIST       18  /* 列出文件 */

/* 键盘输入调用 */
#define SYS_GETCHAR     19  /* 获取键盘字符 */
#define SYS_GETLINE     20  /* 获取一行输入 */

/* IPC 系统调用 */
#define SYS_IPC_SEND    21  /* 发送消息 */
#define SYS_IPC_RECV    22  /* 接收消息 (阻塞) */
#define SYS_IPC_RECV_NB 23  /* 非阻塞接收 */
#define SYS_IPC_CALL    24  /* 同步调用 (send + recv) */
#define SYS_IPC_REPLY   25  /* 回复消息 */

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

#define syscall4(num, arg1, arg2, arg3, arg4) ({ \
    uint32_t ret; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4) \
    ); \
    ret; \
})

#define syscall5(num, arg1, arg2, arg3, arg4, arg5) ({ \
    uint32_t ret; \
    __asm__ volatile( \
        "int $0x80" \
        : "=a"(ret) \
        : "a"(num), "b"(arg1), "c"(arg2), "d"(arg3), "S"(arg4), "D"(arg5) \
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

static inline int sys_exec(const char *filename) {
    return syscall1(SYS_EXEC, (uint32_t)filename);
}

static inline int sys_wait(int *status) {
    return syscall1(SYS_WAIT, (uint32_t)status);
}

/* 文件系统调用封装 */
static inline int sys_open(const char *name, int mode) {
    return syscall2(SYS_OPEN, (uint32_t)name, mode);
}

static inline int sys_close(int fd) {
    return syscall1(SYS_CLOSE, fd);
}

static inline int sys_fread(int fd, void *buf, uint32_t count) {
    return syscall3(SYS_FREAD, fd, (uint32_t)buf, count);
}

static inline int sys_fwrite(int fd, const void *buf, uint32_t count) {
    return syscall3(SYS_FWRITE, fd, (uint32_t)buf, count);
}

static inline int sys_fcreate(const char *name) {
    return syscall1(SYS_FCREATE, (uint32_t)name);
}

static inline int sys_fdelete(const char *name) {
    return syscall1(SYS_FDELETE, (uint32_t)name);
}

static inline int sys_fsize(int fd) {
    return syscall1(SYS_FSIZE, fd);
}

static inline int sys_flist(void) {
    return syscall0(SYS_FLIST);
}

/* 键盘输入调用封装 */
static inline char sys_getchar(void) {
    return (char)syscall0(SYS_GETCHAR);
}

static inline int sys_getline(char *buf, int max) {
    return syscall2(SYS_GETLINE, (uint32_t)buf, max);
}

/* IPC 系统调用封装 */
static inline int sys_ipc_send(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2) {
    return syscall4(SYS_IPC_SEND, target_pid, type, data1, data2);
}

static inline int sys_ipc_recv(uint32_t from_pid, void *msg) {
    return syscall2(SYS_IPC_RECV, from_pid, (uint32_t)msg);
}

static inline int sys_ipc_recv_nb(uint32_t from_pid, void *msg) {
    return syscall2(SYS_IPC_RECV_NB, from_pid, (uint32_t)msg);
}

static inline int sys_ipc_call(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2, void *reply) {
    return syscall5(SYS_IPC_CALL, target_pid, type, data1, data2, (uint32_t)reply);
}

static inline int sys_ipc_reply(uint32_t target_pid, uint32_t type, uint32_t data1, uint32_t data2) {
    return syscall4(SYS_IPC_REPLY, target_pid, type, data1, data2);
}

/* 函数声明 */

/* 初始化系统调用 */
void syscall_init(void);

/* 系统调用处理函数 */
void syscall_handler(struct interrupt_frame *frame);

#endif
