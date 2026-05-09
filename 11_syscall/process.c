/* process.c - 进程管理实现 */

#include <stddef.h>
#include "process.h"
#include "allocator.h"
#include "tss.h"
#include "idt.h"
#include "syscall.h"

/* 进程表 */
static struct process *process_table[MAX_PROCESSES];

/* 当前运行的进程 */
struct process *current_process = NULL;

/* 就绪队列 */
static struct process *ready_queue = NULL;

/* 下一个可用的进程栈区域 */
static uint32_t next_stack_base = 0x200000;

/* 分配 PID */
static uint32_t allocate_pid(void) {
    for (uint32_t i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i] == NULL) {
            return i;
        }
    }
    return 0;  /* 无可用 PID */
}

/* 分配进程栈空间 */
static uint32_t allocate_process_stack(void) {
    uint32_t base = next_stack_base;
    next_stack_base += KERNEL_STACK_SIZE + PROCESS_STACK_SIZE;
    return base;
}

/* 初始化进程管理 */
void process_init(void) {
    /* 清空进程表 */
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i] = NULL;
    }

    current_process = NULL;
    ready_queue = NULL;
}

/* 创建新进程 */
struct process* process_create(void (*entry)(void), uint32_t priority) {
    /* 分配 PID */
    uint32_t pid = allocate_pid();
    if (pid == 0) {
        return NULL;  /* 进程表已满 */
    }

    /* 分配 PCB */
    struct process *proc = (struct process *)kmalloc(sizeof(struct process));
    if (proc == NULL) {
        return NULL;
    }

    /* 初始化 PCB */
    proc->pid = pid;
    proc->state = PROCESS_READY;
    proc->priority = priority;
    proc->parent = NULL;
    proc->first_child = NULL;
    proc->next_sibling = NULL;
    proc->next = NULL;

    /* 分配栈空间 */
    uint32_t stack_base = allocate_process_stack();
    proc->kernel_stack = stack_base + KERNEL_STACK_SIZE;  /* 内核栈顶 */
    proc->user_stack = stack_base + KERNEL_STACK_SIZE + PROCESS_STACK_SIZE;  /* 用户栈顶 */

    /* 设置时间片 */
    proc->time_slice = DEFAULT_TIME_SLICE;
    proc->ticks_remaining = proc->time_slice;

    /* 初始化上下文 */
    /* 设置用户模式入口点 */
    proc->context.eip = (uint32_t)entry;
    proc->context.cs = 0x1B;  /* 用户代码段 (GDT entry 3, RPL=3) */
    proc->context.eflags = 0x202;  /* IF=1 */
    proc->context.useresp = proc->user_stack;
    proc->context.ss = 0x23;  /* 用户数据段 (GDT entry 4, RPL=3) */

    /* 设置段寄存器 */
    proc->context.ds = 0x23;
    proc->context.es = 0x23;
    proc->context.fs = 0x23;
    proc->context.gs = 0x23;

    /* 清空通用寄存器 */
    proc->context.eax = 0;
    proc->context.ebx = 0;
    proc->context.ecx = 0;
    proc->context.edx = 0;
    proc->context.ebp = 0;
    proc->context.esi = 0;
    proc->context.edi = 0;

    /* 中断号和错误码 */
    proc->context.int_no = 0;
    proc->context.err_code = 0;

    /* 加入进程表 */
    process_table[pid] = proc;

    /* 加入就绪队列 */
    if (ready_queue == NULL) {
        ready_queue = proc;
    } else {
        struct process *p = ready_queue;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = proc;
    }

    return proc;
}

/* 销毁进程 */
void process_destroy(struct process *proc) {
    if (proc == NULL) {
        return;
    }

    /* 从进程表中移除 */
    process_table[proc->pid] = NULL;

    /* 从就绪队列中移除 */
    if (ready_queue == proc) {
        ready_queue = proc->next;
    } else {
        struct process *p = ready_queue;
        while (p != NULL && p->next != proc) {
            p = p->next;
        }
        if (p != NULL) {
            p->next = proc->next;
        }
    }

    /* 从父进程的子进程列表中移除 */
    if (proc->parent != NULL) {
        struct process *parent = proc->parent;
        if (parent->first_child == proc) {
            parent->first_child = proc->next_sibling;
        } else {
            struct process *sibling = parent->first_child;
            while (sibling != NULL && sibling->next_sibling != proc) {
                sibling = sibling->next_sibling;
            }
            if (sibling != NULL) {
                sibling->next_sibling = proc->next_sibling;
            }
        }
    }

    /* 释放 PCB */
    kfree(proc);
}

/* 获取当前进程 */
struct process* process_get_current(void) {
    return current_process;
}

/* 设置当前进程 */
void process_set_current(struct process *proc) {
    current_process = proc;
    if (proc != NULL) {
        proc->state = PROCESS_RUNNING;
        /* 更新 TSS 的内核栈 */
        tss_set_kernel_stack(proc->kernel_stack);

        /* 从就绪队列中移除 */
        if (ready_queue == proc) {
            ready_queue = proc->next;
            proc->next = NULL;
        } else {
            struct process *p = ready_queue;
            while (p != NULL && p->next != proc) {
                p = p->next;
            }
            if (p != NULL) {
                p->next = proc->next;
                proc->next = NULL;
            }
        }
    }
}

/* 查找进程 */
struct process* process_find_by_pid(uint32_t pid) {
    if (pid >= MAX_PROCESSES) {
        return NULL;
    }
    return process_table[pid];
}

/* 初始化调度器 */
void scheduler_init(void) {
    process_init();
}

/* 调度 - 从中断上下文切换进程 */
/* 参数 frame 指向当前栈上的中断帧，我们将修改它以切换到新进程 */
void schedule_from_interrupt(struct interrupt_frame *frame) {
    /* 保存当前进程的上下文 */
    if (current_process != NULL && current_process->state == PROCESS_RUNNING) {
        /* 复制当前栈上的中断帧到当前进程的 PCB */
        current_process->context = *frame;

        /* 将当前进程放回就绪队列尾部 */
        current_process->state = PROCESS_READY;
        current_process->ticks_remaining = current_process->time_slice;
        current_process->next = NULL;

        /* 加入就绪队列尾部 */
        if (ready_queue == NULL) {
            ready_queue = current_process;
        } else {
            struct process *p = ready_queue;
            while (p->next != NULL) {
                p = p->next;
            }
            p->next = current_process;
        }
    }

    /* 如果没有就绪进程，继续运行当前进程 */
    if (ready_queue == NULL) {
        if (current_process != NULL) {
            current_process->ticks_remaining = current_process->time_slice;
        }
        return;
    }

    /* 从就绪队列取出新进程 */
    struct process *new_proc = ready_queue;
    ready_queue = new_proc->next;
    new_proc->next = NULL;
    new_proc->state = PROCESS_RUNNING;

    /* 更新当前进程指针 */
    current_process = new_proc;

    /* 更新 TSS 内核栈 */
    tss_set_kernel_stack(new_proc->kernel_stack);

    /* 复制新进程的上下文到当前栈上的中断帧 */
    /* 这样 iret 就会恢复到新进程的上下文 */
    *frame = new_proc->context;
}

/* 调度 - 选择下一个进程运行 */
void schedule(void) {
    /* 这个函数不应该直接调用，应该通过 schedule_from_interrupt */
    if (current_process != NULL) {
        current_process->ticks_remaining = 0;
    }
}

/* 主动让出 CPU */
void yield(void) {
    if (current_process != NULL) {
        current_process->ticks_remaining = 0;
        schedule();
    }
}

/* 阻塞进程 */
void process_block(struct process *proc) {
    if (proc == NULL) {
        return;
    }
    proc->state = PROCESS_BLOCKED;

    /* 从就绪队列移除 */
    if (ready_queue == proc) {
        ready_queue = proc->next;
        proc->next = NULL;
    } else {
        struct process *p = ready_queue;
        while (p != NULL && p->next != proc) {
            p = p->next;
        }
        if (p != NULL) {
            p->next = proc->next;
            proc->next = NULL;
        }
    }
}

/* 唤醒进程 */
void process_unblock(struct process *proc) {
    if (proc == NULL) {
        return;
    }
    proc->state = PROCESS_READY;

    /* 加入就绪队列 */
    proc->next = NULL;
    if (ready_queue == NULL) {
        ready_queue = proc;
    } else {
        struct process *p = ready_queue;
        while (p->next != NULL) {
            p = p->next;
        }
        p->next = proc;
    }
}
