/* process.h - 进程管理头文件 */

#ifndef PROCESS_H
#define PROCESS_H

#include <stdint.h>

/* 进程状态 */
typedef enum {
    PROCESS_CREATED = 0,    /* 新创建 */
    PROCESS_READY,          /* 就绪态 */
    PROCESS_RUNNING,        /* 运行态 */
    PROCESS_BLOCKED,        /* 阻塞态 */
    PROCESS_TERMINATED      /* 已终止 */
} process_state_t;

/* 进程优先级 */
typedef enum {
    PRIORITY_LOW = 0,
    PRIORITY_NORMAL = 1,
    PRIORITY_HIGH = 2
} process_priority_t;

/* 进程控制块 (PCB) */
typedef struct process {
    uint32_t pid;                   /* 进程 ID */
    char name[16];                  /* 进程名称 */
    process_state_t state;          /* 进程状态 */
    process_priority_t priority;    /* 优先级 */

    /* 寄存器状态（用于上下文切换） */
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t eip;
    uint32_t eflags;

    /* 栈信息 */
    uint32_t stack_top;             /* 栈顶地址 */
    uint32_t stack_size;            /* 栈大小 */

    /* 内存信息 */
    uint32_t heap_start;            /* 堆起始地址 */
    uint32_t heap_size;             /* 堆大小 */

    /* 链表指针 */
    struct process *next;
    struct process *prev;
} process_t;

/* 最大进程数 */
#define MAX_PROCESS 16

/* 默认栈大小 */
#define DEFAULT_STACK_SIZE 4096

/* 函数声明 */

/* 初始化进程管理 */
void process_init(void);

/* 创建进程 */
process_t* process_create(const char *name, void (*entry_point)(void), process_priority_t priority);

/* 终止进程 */
void process_exit(process_t *proc);

/* 获取当前进程 */
process_t* process_get_current(void);

/* 获取下一个就绪进程 */
process_t* process_get_next_ready(void);

/* 上下文切换 */
void process_switch(process_t *from, process_t *to);

/* 进程调度器 */
void process_schedule(void);

/* 获取进程数量 */
int process_get_count(void);

/* 打印进程列表 */
void process_print_list(void);

/* 进程 ID */
uint32_t process_get_next_pid(void);

#endif