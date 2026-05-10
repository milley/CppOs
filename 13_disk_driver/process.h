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

/* 进程控制块 */
struct process {
    uint32_t pid;
    uint32_t state;
    uint32_t priority;

    struct interrupt_frame context;

    uint32_t kernel_stack;
    uint32_t user_stack;
    address_space_t *address_space;  /* 进程地址空间 */

    uint32_t time_slice;
    uint32_t ticks_remaining;

    struct process *parent;
    struct process *first_child;
    struct process *next_sibling;
    struct process *next;
};

#define MAX_PROCESSES 64
#define DEFAULT_TIME_SLICE 5
#define KERNEL_STACK_SIZE 0x10000
#define PROCESS_STACK_SIZE 0x80000

void process_init(void);
struct process* process_create(void (*entry)(void), uint32_t priority);
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

#endif