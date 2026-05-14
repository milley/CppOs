/* process.h - 进程管理头文件 */

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>
#include "idt.h"
#include "paging.h"

/* 进程状态 */
#define PROCESS_READY    0
#define PROCESS_RUNNING  1
#define PROCESS_BLOCKED  2
#define PROCESS_ZOMBIE   3

/* 进程优先级 */
#define PRIORITY_LOW     0
#define PRIORITY_NORMAL  1
#define PRIORITY_HIGH    2

/* 进程限制 */
#define MAX_PROCESSES    64
#define MAX_OPEN_FILES   8

/* 默认时间片 */
#define DEFAULT_TIME_SLICE 5

/* 栈大小 */
#define KERNEL_STACK_SIZE  0x20000   /* 128KB kernel stack */
#define USER_STACK_SIZE    0x80000   /* 512KB user stack */

/* 栈基地址 */
#define KERNEL_STACK_BASE  0x300000
#define USER_STACK_BASE    0x400000

/* 进程控制块 */
struct process {
    uint32_t pid;
    uint32_t state;
    uint32_t priority;

    struct interrupt_frame context;

    uint32_t kernel_stack;
    uint32_t user_stack;
    uint32_t user_stack_top;      /* 栈顶地址 (用于fork) */
    address_space_t *address_space;

    uint32_t time_slice;
    uint32_t ticks_remaining;

    /* 退出状态 */
    int32_t exit_status;

    /* 打开的文件描述符 */
    int open_files[MAX_OPEN_FILES];

    /* IPC 相关 */
    struct message *msg_queue;        /* 消息队列头 */
    struct message *msg_queue_tail;   /* 消息队列尾 */
    uint32_t waiting_for_sender;      /* 等待来自特定 PID 的消息 (0=任意) */

    /* 进程树 */
    struct process *parent;
    struct process *first_child;
    struct process *next_sibling;
    struct process *next;
};

void process_init(void);
struct process* process_create(void (*entry)(void), uint32_t priority);
struct process* process_create_kernel(void (*entry)(void), uint32_t priority);
void process_destroy(struct process *proc);
struct process* process_get_current(void);
void process_set_current(struct process *proc);
struct process* process_find_by_pid(uint32_t pid);

void scheduler_init(void);
void schedule(void);
void schedule_from_interrupt(struct interrupt_frame *frame);
void yield(void);

void process_block(struct process *proc);
void process_unblock(struct process *proc);
void switch_to_first(struct process *proc);

/* 进程资源管理 */
void process_close_all_files(struct process *proc);
void process_reparent_children(struct process *proc);

/* 进程表管理 */
void process_register(struct process *proc);
void process_unregister(struct process *proc);

#endif