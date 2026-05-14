/* process.c - 进程管理实现 */

#include <stddef.h>
#include "process.h"
#include "allocator.h"
#include "tss.h"
#include "idt.h"
#include "syscall.h"
#include "paging.h"

static struct process *process_table[MAX_PROCESSES];
struct process *current_process = NULL;
static struct process *ready_queue = NULL;

static uint32_t allocate_pid(void) {
    for (uint32_t i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i] == NULL) return i;
    }
    return 0;
}

void process_init(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i] = NULL;
    }
    current_process = NULL;
    ready_queue = NULL;
}

struct process* process_create(void (*entry)(void), uint32_t priority) {
    uint32_t pid = allocate_pid();
    if (pid == 0) return NULL;

    struct process *proc = (struct process*)kmalloc(sizeof(struct process));
    if (proc == NULL) return NULL;

    /* 创建地址空间 */
    proc->address_space = address_space_create();
    if (proc->address_space == NULL) {
        kfree(proc);
        return NULL;
    }

    /* 基于 PID 分配栈 - 每个进程独立的栈空间 */
    proc->kernel_stack = KERNEL_STACK_BASE + pid * KERNEL_STACK_SIZE + KERNEL_STACK_SIZE;
    proc->user_stack = USER_STACK_BASE + pid * USER_STACK_SIZE + USER_STACK_SIZE;
    proc->user_stack_top = proc->user_stack;

    proc->pid = pid;
    proc->state = PROCESS_READY;
    proc->priority = priority;
    proc->exit_status = 0;

    /* 初始化文件描述符表 */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        proc->open_files[i] = -1;
    }

    proc->parent = NULL;
    proc->first_child = NULL;
    proc->next_sibling = NULL;
    proc->next = NULL;
    proc->time_slice = DEFAULT_TIME_SLICE;
    proc->ticks_remaining = proc->time_slice;

    /* 初始化 IPC 字段 */
    proc->msg_queue = NULL;
    proc->msg_queue_tail = NULL;
    proc->waiting_for_sender = 0;

    proc->context.eip = (uint32_t)entry;
    proc->context.cs = 0x1B;
    proc->context.eflags = 0x202;
    proc->context.useresp = proc->user_stack;
    proc->context.ss = 0x23;
    proc->context.ds = 0x23;
    proc->context.es = 0x23;
    proc->context.fs = 0x23;
    proc->context.gs = 0x23;
    proc->context.eax = 0;
    proc->context.ebx = 0;
    proc->context.ecx = 0;
    proc->context.edx = 0;
    proc->context.ebp = 0;
    proc->context.esi = 0;
    proc->context.edi = 0;
    proc->context.int_no = 0;
    proc->context.err_code = 0;

    process_table[pid] = proc;

    if (ready_queue == NULL) {
        ready_queue = proc;
    } else {
        struct process *p = ready_queue;
        while (p->next != NULL) p = p->next;
        p->next = proc;
    }

    return proc;
}

/* 创建内核态进程 */
struct process* process_create_kernel(void (*entry)(void), uint32_t priority) {
    uint32_t pid = allocate_pid();
    if (pid == 0) return NULL;

    struct process *proc = (struct process*)kmalloc(sizeof(struct process));
    if (proc == NULL) return NULL;

    /* 内核态进程不需要独立的地址空间 */
    proc->address_space = NULL;

    /* 基于 PID 分配内核栈 */
    proc->kernel_stack = KERNEL_STACK_BASE + pid * KERNEL_STACK_SIZE + KERNEL_STACK_SIZE;
    proc->user_stack = 0;  /* 内核态进程不使用用户栈 */
    proc->user_stack_top = 0;

    proc->pid = pid;
    proc->state = PROCESS_READY;
    proc->priority = priority;
    proc->exit_status = 0;

    /* 初始化文件描述符表 */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        proc->open_files[i] = -1;
    }

    proc->parent = NULL;
    proc->first_child = NULL;
    proc->next_sibling = NULL;
    proc->next = NULL;
    proc->time_slice = DEFAULT_TIME_SLICE;
    proc->ticks_remaining = proc->time_slice;

    /* 初始化 IPC 字段 */
    proc->msg_queue = NULL;
    proc->msg_queue_tail = NULL;
    proc->waiting_for_sender = 0;

    /* 内核态进程上下文 */
    proc->context.eip = (uint32_t)entry;
    proc->context.cs = 0x08;        /* 内核代码段 */
    proc->context.eflags = 0x202;
    proc->context.useresp = proc->kernel_stack;  /* 使用内核栈 */
    proc->context.ss = 0x10;        /* 内核数据段 */
    proc->context.ds = 0x10;
    proc->context.es = 0x10;
    proc->context.fs = 0x10;
    proc->context.gs = 0x10;
    proc->context.eax = 0;
    proc->context.ebx = 0;
    proc->context.ecx = 0;
    proc->context.edx = 0;
    proc->context.ebp = 0;
    proc->context.esi = 0;
    proc->context.edi = 0;
    proc->context.int_no = 0;
    proc->context.err_code = 0;

    process_table[pid] = proc;

    if (ready_queue == NULL) {
        ready_queue = proc;
    } else {
        struct process *p = ready_queue;
        while (p->next != NULL) p = p->next;
        p->next = proc;
    }

    return proc;
}

void process_destroy(struct process *proc) {
    if (proc == NULL) return;

    process_table[proc->pid] = NULL;

    if (ready_queue == proc) {
        ready_queue = proc->next;
    } else {
        struct process *p = ready_queue;
        while (p != NULL && p->next != proc) p = p->next;
        if (p != NULL) p->next = proc->next;
    }

    if (proc->parent != NULL) {
        struct process *parent = proc->parent;
        if (parent->first_child == proc) {
            parent->first_child = proc->next_sibling;
        } else {
            struct process *s = parent->first_child;
            while (s != NULL && s->next_sibling != proc) s = s->next_sibling;
            if (s != NULL) s->next_sibling = proc->next_sibling;
        }
    }

    if (proc->address_space != NULL) {
        address_space_destroy(proc->address_space);
    }

    kfree(proc);
}

struct process* process_get_current(void) {
    return current_process;
}

void process_set_current(struct process *proc) {
    current_process = proc;
    if (proc != NULL) {
        proc->state = PROCESS_RUNNING;
        tss_set_kernel_stack(proc->kernel_stack);

        if (proc->address_space != NULL) {
            address_space_switch(proc->address_space);
        }

        if (ready_queue == proc) {
            ready_queue = proc->next;
            proc->next = NULL;
        } else {
            struct process *p = ready_queue;
            while (p != NULL && p->next != proc) p = p->next;
            if (p != NULL) p->next = proc->next;
            proc->next = NULL;
        }
    }
}

struct process* process_find_by_pid(uint32_t pid) {
    if (pid >= MAX_PROCESSES) return NULL;
    return process_table[pid];
}

void scheduler_init(void) {
    process_init();
}

void schedule_from_interrupt(struct interrupt_frame *frame) {
    if (current_process != NULL && current_process->state == PROCESS_RUNNING) {
        current_process->context = *frame;
        current_process->state = PROCESS_READY;
        current_process->ticks_remaining = current_process->time_slice;
        current_process->next = NULL;

        if (ready_queue == NULL) {
            ready_queue = current_process;
        } else {
            struct process *p = ready_queue;
            while (p->next != NULL) p = p->next;
            p->next = current_process;
        }
    }

    if (ready_queue == NULL) {
        if (current_process != NULL) {
            current_process->ticks_remaining = current_process->time_slice;
        }
        return;
    }

    struct process *new_proc = ready_queue;
    ready_queue = new_proc->next;
    new_proc->next = NULL;
    new_proc->state = PROCESS_RUNNING;

    current_process = new_proc;
    tss_set_kernel_stack(new_proc->kernel_stack);

    if (new_proc->address_space != NULL) {
        address_space_switch(new_proc->address_space);
    }

    *frame = new_proc->context;
}

void schedule(void) {
    if (current_process != NULL) {
        current_process->ticks_remaining = 0;
    }
}

void yield(void) {
    if (current_process != NULL) {
        current_process->ticks_remaining = 0;
        schedule();
    }
}

void process_block(struct process *proc) {
    if (proc == NULL) return;
    proc->state = PROCESS_BLOCKED;

    if (ready_queue == proc) {
        ready_queue = proc->next;
        proc->next = NULL;
    } else {
        struct process *p = ready_queue;
        while (p != NULL && p->next != proc) p = p->next;
        if (p != NULL) p->next = proc->next;
        proc->next = NULL;
    }
}

void process_unblock(struct process *proc) {
    if (proc == NULL) return;
    proc->state = PROCESS_READY;
    proc->next = NULL;
    if (ready_queue == NULL) {
        ready_queue = proc;
    } else {
        struct process *p = ready_queue;
        while (p->next != NULL) p = p->next;
        p->next = proc;
    }
}

/* 关闭进程所有打开的文件 */
void process_close_all_files(struct process *proc) {
    if (proc == NULL) return;

    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (proc->open_files[i] >= 0) {
            /* 这里需要调用 fs_close，但要避免循环依赖 */
            /* 在 syscall.c 中处理 */
            proc->open_files[i] = -1;
        }
    }
}

/* 重新设置子进程的父进程 (当父进程退出时) */
void process_reparent_children(struct process *proc) {
    if (proc == NULL) return;

    struct process *child = proc->first_child;
    while (child != NULL) {
        struct process *next = child->next_sibling;

        /* 将子进程的父进程设为 init (PID 1) */
        struct process *init = process_find_by_pid(1);
        if (init != NULL) {
            child->parent = init;
            child->next_sibling = init->first_child;
            init->first_child = child;
        }

        /* 如果子进程是僵尸状态，唤醒 init */
        if (child->state == PROCESS_ZOMBIE && init != NULL && init->state == PROCESS_BLOCKED) {
            process_unblock(init);
        }

        child = next;
    }
    proc->first_child = NULL;
}

/* 注册进程到进程表 */
void process_register(struct process *proc) {
    if (proc == NULL || proc->pid >= MAX_PROCESSES) return;
    process_table[proc->pid] = proc;
}

/* 从进程表移除进程 */
void process_unregister(struct process *proc) {
    if (proc == NULL || proc->pid >= MAX_PROCESSES) return;
    process_table[proc->pid] = NULL;
}
