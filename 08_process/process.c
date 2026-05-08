/* process.c - 进程管理实现 */

#include "process.h"
#include "allocator.h"

/* 进程表 */
static process_t process_table[MAX_PROCESS];

/* 当前运行进程 */
static process_t *current_process = 0;

/* 进程链表 */
static process_t *process_list = 0;

/* 就绪队列 */
static process_t *ready_queue = 0;

/* 下一个 PID */
static uint32_t next_pid = 1;

/* 进程数量 */
static int process_count = 0;

/* 初始化进程管理 */
void process_init(void) {
    /* 清空进程表 */
    for (int i = 0; i < MAX_PROCESS; i++) {
        process_table[i].pid = 0;
        process_table[i].state = PROCESS_TERMINATED;
        process_table[i].next = 0;
        process_table[i].prev = 0;
    }

    current_process = 0;
    process_list = 0;
    ready_queue = 0;
    next_pid = 1;
    process_count = 0;
}

/* 获取下一个 PID */
uint32_t process_get_next_pid(void) {
    return next_pid++;
}

/* 获取空闲进程槽 */
static process_t* get_free_slot(void) {
    for (int i = 0; i < MAX_PROCESS; i++) {
        if (process_table[i].pid == 0) {
            return &process_table[i];
        }
    }
    return 0;
}

/* 添加到就绪队列（简单 FIFO） */
static void add_to_ready_queue(process_t *proc) {
    proc->state = PROCESS_READY;

    if (ready_queue == 0) {
        ready_queue = proc;
        proc->next = 0;
        proc->prev = 0;
    } else {
        /* 添加到队尾 */
        process_t *current = ready_queue;
        while (current->next) {
            current = current->next;
        }
        current->next = proc;
        proc->prev = current;
        proc->next = 0;
    }
}

/* 从就绪队列移除 */
static void remove_from_ready_queue(process_t *proc) {
    if (proc->prev) {
        proc->prev->next = proc->next;
    } else {
        ready_queue = proc->next;
    }

    if (proc->next) {
        proc->next->prev = proc->prev;
    }

    proc->next = 0;
    proc->prev = 0;
}

/* 创建进程 */
process_t* process_create(const char *name, void (*entry_point)(void), process_priority_t priority) {
    if (process_count >= MAX_PROCESS) {
        return 0;
    }

    /* 获取空闲槽 */
    process_t *proc = get_free_slot();
    if (!proc) {
        return 0;
    }

    /* 设置基本信息 */
    proc->pid = process_get_next_pid();

    /* 复制名称 */
    int i = 0;
    while (name[i] && i < 15) {
        proc->name[i] = name[i];
        i++;
    }
    proc->name[i] = '\0';

    proc->state = PROCESS_CREATED;
    proc->priority = priority;

    /* 分配栈 */
    void *stack = kmalloc(DEFAULT_STACK_SIZE);
    if (!stack) {
        proc->pid = 0;
        return 0;
    }

    proc->stack_top = (uint32_t)stack + DEFAULT_STACK_SIZE;
    proc->stack_size = DEFAULT_STACK_SIZE;

    /* 初始化栈 - 模拟 context_switch 的栈布局 */
    uint32_t *stack_ptr = (uint32_t *)proc->stack_top;

    /* 栈布局（从高到低）：
     * [entry_point]  <- ret 会跳转到这里
     * [eax=0]
     * [ebx=0]
     * [ecx=0]
     * [edx=0]
     * [esi=0]
     * [edi=0]
     * [ebp=0]
     */

    stack_ptr--; *stack_ptr = (uint32_t)entry_point;  /* 返回地址 = 入口点 */
    stack_ptr--; *stack_ptr = 0;          /* eax */
    stack_ptr--; *stack_ptr = 0;          /* ebx */
    stack_ptr--; *stack_ptr = 0;          /* ecx */
    stack_ptr--; *stack_ptr = 0;          /* edx */
    stack_ptr--; *stack_ptr = 0;          /* esi */
    stack_ptr--; *stack_ptr = 0;          /* edi */
    stack_ptr--; *stack_ptr = 0;          /* ebp */

    proc->esp = (uint32_t)stack_ptr;
    proc->eip = (uint32_t)entry_point;

    /* 添加到就绪队列 */
    add_to_ready_queue(proc);

    process_count++;

    return proc;
}

/* 终止进程 */
void process_exit(process_t *proc) {
    if (!proc) {
        return;
    }

    proc->state = PROCESS_TERMINATED;

    /* 从就绪队列移除 */
    remove_from_ready_queue(proc);

    /* 释放栈 */
    if (proc->stack_top && proc->stack_size) {
        void *stack = (void *)(proc->stack_top - proc->stack_size);
        kfree(stack);
    }

    proc->pid = 0;
    process_count--;

    /* 如果是当前进程，触发调度 */
    if (proc == current_process) {
        current_process = 0;
        process_schedule();
    }
}

/* 获取当前进程 */
process_t* process_get_current(void) {
    return current_process;
}

/* 获取下一个就绪进程 */
process_t* process_get_next_ready(void) {
    return ready_queue;
}

/* 获取进程数量 */
int process_get_count(void) {
    return process_count;
}

/* 上下文切换（汇编实现） */
extern void context_switch(uint32_t *old_sp, uint32_t new_sp);

/* 进程调度 */
void process_schedule(void) {
    process_t *next = process_get_next_ready();

    if (!next) {
        return;  /* 没有可运行的进程 */
    }

    process_t *prev = current_process;

    /* 从就绪队列移除 */
    remove_from_ready_queue(next);

    /* 设置新进程为运行态 */
    next->state = PROCESS_RUNNING;
    current_process = next;

    if (prev) {
        /* 保存当前进程状态，放回就绪队列 */
        prev->state = PROCESS_READY;
        add_to_ready_queue(prev);

        /* 执行上下文切换 */
        context_switch(&prev->esp, next->esp);

        /* 当其他进程切换回来时，会从这里继续执行 */
        return;
    }

    /* 第一次运行，直接加载进程状态 */
    context_switch(0, next->esp);
}
