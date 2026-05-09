/* process.h - 进程管理头文件 */

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "idt.h"

/* 进程状态 */
#define PROCESS_READY    0    /* 就绪 */
#define PROCESS_RUNNING  1    /* 运行中 */
#define PROCESS_BLOCKED  2    /* 阻塞 */
#define PROCESS_ZOMBIE   3    /* 僵尸状态 */

/* 进程优先级 */
#define PRIORITY_LOW     0
#define PRIORITY_NORMAL  1
#define PRIORITY_HIGH    2

/* 进程控制块 (PCB) */
struct process {
    uint32_t pid;                    /* 进程 ID */
    uint32_t state;                  /* 进程状态 */
    uint32_t priority;               /* 优先级 */

    /* CPU 上下文 */
    struct interrupt_frame context;  /* 保存的寄存器状态 */

    /* 内存管理 */
    uint32_t kernel_stack;           /* 内核栈顶 (ESP0) */
    uint32_t user_stack;             /* 用户栈顶 */

    /* 调度相关 */
    uint32_t time_slice;             /* 时间片大小 */
    uint32_t ticks_remaining;        /* 剩余时间片 */

    /* 进程树 */
    struct process *parent;          /* 父进程 */
    struct process *first_child;     /* 第一个子进程 */
    struct process *next_sibling;    /* 下一个兄弟进程 */

    /* 链表 */
    struct process *next;            /* 就绪队列下一个 */
};

/* 最大进程数 */
#define MAX_PROCESSES 64

/* 默认时间片 */
#define DEFAULT_TIME_SLICE 5

/* 内核栈大小 */
#define KERNEL_STACK_SIZE 0x10000   /* 64KB */

/* 进程用户栈大小 */
#define PROCESS_STACK_SIZE 0x80000  /* 512KB */

/* 函数声明 */

/* 初始化进程管理 */
void process_init(void);

/* 创建新进程 */
struct process* process_create(void (*entry)(void), uint32_t priority);

/* 销毁进程 */
void process_destroy(struct process *proc);

/* 获取当前进程 */
struct process* process_get_current(void);

/* 设置当前进程 */
void process_set_current(struct process *proc);

/* 查找进程 */
struct process* process_find_by_pid(uint32_t pid);

/* 调度器 */
void scheduler_init(void);
void schedule(void);
void schedule_from_interrupt(struct interrupt_frame *frame);
void yield(void);

/* 进程阻塞/唤醒 */
void process_block(struct process *proc);
void process_unblock(struct process *proc);

/* 上下文切换 */
void switch_to_first(struct process *proc);

#endif
