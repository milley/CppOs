/* syscall.c - 系统调用实现 */

#include <stddef.h>
#include "syscall.h"
#include "idt.h"
#include "process.h"
#include "allocator.h"
#include "tss.h"
#include "filesystem.h"
#include "keyboard.h"
#include "string.h"

/* 时钟计数（外部定义） */
extern uint32_t timer_ticks;

/* 系统调用：退出进程 */
static uint32_t sys_exit_handler(int status) {
    struct process *proc = process_get_current();
    if (proc == NULL) {
        while (1) {
            __asm__ volatile("hlt");
        }
    }

    /* 保存退出状态 */
    proc->exit_status = status;

    /* 关闭所有打开的文件 */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (proc->open_files[i] >= 0) {
            fs_close(proc->open_files[i]);
            proc->open_files[i] = -1;
        }
    }

    /* 将子进程重新设置为 init 的子进程 */
    process_reparent_children(proc);

    /* 设置为僵尸状态 */
    proc->state = PROCESS_ZOMBIE;
    proc->ticks_remaining = 0;

    /* 如果父进程在等待，唤醒它 */
    if (proc->parent != NULL && proc->parent->state == PROCESS_BLOCKED) {
        process_unblock(proc->parent);
    }

    /* 触发调度 */
    schedule();

    /* 不应该到达这里 */
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
    struct process *proc = process_get_current();
    if (proc != NULL) {
        return proc->pid;
    }
    return 0;
}

/* 系统调用：wait - 等待子进程 */
static uint32_t sys_wait_handler(uint32_t *status) {
    struct process *parent = process_get_current();
    if (parent == NULL) {
        return (uint32_t)-1;
    }

wait_again:
    /* 检查是否有子进程 */
    if (parent->first_child == NULL) {
        return (uint32_t)-1;
    }

    /* 查找僵尸子进程 */
    struct process *child = parent->first_child;
    struct process *prev = NULL;

    while (child != NULL) {
        if (child->state == PROCESS_ZOMBIE) {
            /* 找到僵尸进程，回收 */
            uint32_t child_pid = child->pid;

            /* 保存退出状态 */
            if (status != NULL) {
                *status = child->exit_status;
            }

            /* 从子进程列表移除 */
            if (prev == NULL) {
                parent->first_child = child->next_sibling;
            } else {
                prev->next_sibling = child->next_sibling;
            }

            /* 从进程表中移除 */
            process_find_by_pid(child->pid);  /* 确保 PID 有效 */
            kfree(child);

            return child_pid;
        }
        prev = child;
        child = child->next_sibling;
    }

    /* 没有僵尸子进程，阻塞等待 */
    process_block(parent);
    schedule();

    /* 被唤醒后再次检查 */
    goto wait_again;
}

/* ==================== 文件系统调用 ==================== */

/* 系统调用：打开文件 */
static uint32_t sys_open_handler(const char *name, uint32_t mode, uint32_t unused) {
    (void)unused;
    if (name == NULL) return (uint32_t)-1;
    return (uint32_t)fs_open(name, (int)mode);
}

/* 系统调用：关闭文件 */
static uint32_t sys_close_handler(uint32_t fd, uint32_t unused1, uint32_t unused2) {
    (void)unused1;
    (void)unused2;
    return (uint32_t)fs_close((int)fd);
}

/* 系统调用：读取文件 */
static uint32_t sys_fread_handler(uint32_t fd, uint32_t buf, uint32_t count) {
    if (buf == 0 || count == 0) return 0;
    return (uint32_t)fs_read((int)fd, (void *)buf, count);
}

/* 系统调用：写入文件 */
static uint32_t sys_fwrite_handler(uint32_t fd, uint32_t buf, uint32_t count) {
    if (buf == 0 || count == 0) return 0;
    return (uint32_t)fs_write((int)fd, (const void *)buf, count);
}

/* 系统调用：创建文件 */
static uint32_t sys_fcreate_handler(const char *name, uint32_t unused1, uint32_t unused2) {
    (void)unused1;
    (void)unused2;
    if (name == NULL) return (uint32_t)-1;
    return (uint32_t)fs_create(name);
}

/* 系统调用：删除文件 */
static uint32_t sys_fdelete_handler(const char *name, uint32_t unused1, uint32_t unused2) {
    (void)unused1;
    (void)unused2;
    if (name == NULL) return (uint32_t)-1;
    return (uint32_t)fs_delete(name);
}

/* 系统调用：获取文件大小 */
static uint32_t sys_fsize_handler(uint32_t fd, uint32_t unused1, uint32_t unused2) {
    (void)unused1;
    (void)unused2;
    return (uint32_t)fs_size((int)fd);
}

/* 系统调用：列出文件 */
static uint32_t sys_flist_handler(uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1;
    (void)unused2;
    (void)unused3;
    return (uint32_t)fs_list();
}

/* ==================== 键盘输入调用 ==================== */

/* 系统调用：获取键盘字符 */
static uint32_t sys_getchar_handler(uint32_t unused1, uint32_t unused2, uint32_t unused3) {
    (void)unused1;
    (void)unused2;
    (void)unused3;
    return (uint32_t)(uint8_t)keyboard_getchar();
}

/* 系统调用：获取一行输入 */
static uint32_t sys_getline_handler(uint32_t buf, uint32_t max, uint32_t unused) {
    (void)unused;
    if (buf == 0 || max == 0) return 0;
    return (uint32_t)keyboard_getline((char *)buf, (int)max);
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
    [SYS_WAIT]     = (syscall_func_t)sys_wait_handler,
    /* 文件系统调用 */
    [SYS_OPEN]     = (syscall_func_t)sys_open_handler,
    [SYS_CLOSE]    = (syscall_func_t)sys_close_handler,
    [SYS_FREAD]    = (syscall_func_t)sys_fread_handler,
    [SYS_FWRITE]   = (syscall_func_t)sys_fwrite_handler,
    [SYS_FCREATE]  = (syscall_func_t)sys_fcreate_handler,
    [SYS_FDELETE]  = (syscall_func_t)sys_fdelete_handler,
    [SYS_FSIZE]    = (syscall_func_t)sys_fsize_handler,
    [SYS_FLIST]    = (syscall_func_t)sys_flist_handler,
    /* 键盘输入调用 */
    [SYS_GETCHAR]  = (syscall_func_t)sys_getchar_handler,
    [SYS_GETLINE]  = (syscall_func_t)sys_getline_handler,
    /* SYS_FORK 和 SYS_EXEC 在 syscall_handler 中特殊处理 */
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

    if (syscall_num == SYS_FORK) {
            /* fork 需要当前上下文 */
            struct process *parent = process_get_current();

            /* 调试输出 - 第 11 行 */
            char *debug_video = (char *)0xB8000;
            int dbg_offset = 11 * 160;
            debug_video[dbg_offset] = 'F';
            debug_video[dbg_offset + 1] = 0x0E;
            debug_video[dbg_offset + 2] = 'O';
            debug_video[dbg_offset + 3] = 0x0E;
            debug_video[dbg_offset + 4] = 'R';
            debug_video[dbg_offset + 5] = 0x0E;
            debug_video[dbg_offset + 6] = 'K';
            debug_video[dbg_offset + 7] = 0x0E;
            debug_video[dbg_offset + 8] = ':';
            debug_video[dbg_offset + 9] = 0x0E;

            if (parent == NULL) {
                /* parent 是 NULL */
                debug_video[dbg_offset + 10] = 'N';
                debug_video[dbg_offset + 11] = 0x0C;
                debug_video[dbg_offset + 12] = 'U';
                debug_video[dbg_offset + 13] = 0x0C;
                debug_video[dbg_offset + 14] = 'L';
                debug_video[dbg_offset + 15] = 0x0C;
                debug_video[dbg_offset + 16] = 'L';
                debug_video[dbg_offset + 17] = 0x0C;
                ret = (uint32_t)-1;
            } else {
                /* parent 存在，显示 PID */
                debug_video[dbg_offset + 10] = 'P';
                debug_video[dbg_offset + 11] = 0x0A;
                debug_video[dbg_offset + 12] = 'I';
                debug_video[dbg_offset + 13] = 0x0A;
                debug_video[dbg_offset + 14] = 'D';
                debug_video[dbg_offset + 15] = 0x0A;
                debug_video[dbg_offset + 16] = '0' + parent->pid;
                debug_video[dbg_offset + 17] = 0x0A;

                /* 分配 PID */
                uint32_t pid = 0;
                for (uint32_t i = 1; i < MAX_PROCESSES; i++) {
                    if (process_find_by_pid(i) == NULL) {
                        pid = i;
                        break;
                    }
                }

                /* 显示找到的 PID */
                debug_video[dbg_offset + 18] = 'N';
                debug_video[dbg_offset + 19] = 0x0B;
                debug_video[dbg_offset + 20] = '0' + pid;
                debug_video[dbg_offset + 21] = 0x0B;

                if (pid == 0) {
                    ret = (uint32_t)-1;
                } else {
                    /* 分配 PCB */
                    struct process *child = (struct process *)kmalloc(sizeof(struct process));
                    if (child == NULL) {
                        debug_video[dbg_offset + 22] = 'M';
                        debug_video[dbg_offset + 23] = 0x0C;
                        ret = (uint32_t)-1;
                    } else {
                        /* 复制父进程的上下文（从中断帧） */
                        child->pid = pid;
                        child->state = PROCESS_READY;
                        child->priority = parent->priority;
                        child->time_slice = parent->time_slice;
                        child->ticks_remaining = child->time_slice;
                        child->exit_status = 0;

                        /* 初始化文件描述符表 */
                        for (int i = 0; i < MAX_OPEN_FILES; i++) {
                            child->open_files[i] = -1;
                        }

                        /* 复制 CPU 上下文 - 直接从中断帧复制 */
                        child->context = *frame;
                        child->context.eax = 0;  /* 子进程返回 0 */

                        /* 分配独立的栈空间 - 使用与 process_create 相同的公式 */
                        child->kernel_stack = KERNEL_STACK_BASE + pid * KERNEL_STACK_SIZE + KERNEL_STACK_SIZE;
                        child->user_stack = USER_STACK_BASE + pid * USER_STACK_SIZE + USER_STACK_SIZE;
                        child->user_stack_top = child->user_stack;  /* 栈顶，向下增长 */

                        /* 地址空间 - 简化版：共享父进程地址空间 */
                        child->address_space = parent->address_space;

                        /* 对于内核态进程，直接使用父进程的栈信息 */
                        /* 子进程会在调度时从 frame 恢复上下文，返回到 sys_fork 调用点 */

                        /* 设置进程树关系 */
                        child->parent = parent;
                        child->first_child = NULL;
                        child->next_sibling = parent->first_child;
                        parent->first_child = child;
                        child->next = NULL;

                        /* 注册到进程表 */
                        process_register(child);

                        /* 加入就绪队列 */
                        process_unblock(child);

                        /* 成功! */
                        debug_video[dbg_offset + 22] = 'O';
                        debug_video[dbg_offset + 23] = 0x0A;
                        debug_video[dbg_offset + 24] = 'K';
                        debug_video[dbg_offset + 25] = 0x0A;

                        /* 父进程返回子进程 PID */
                        ret = pid;
                    }
                }
            }
        } else if (syscall_num == SYS_EXEC) {
            /* exec 需要修改 frame */
            struct process *proc = process_get_current();
            if (proc != NULL) {
                /* 重置上下文 - 直接修改 frame */
                frame->eip = arg1;
                frame->cs = 0x1B;
                frame->eflags = 0x202;
                frame->useresp = proc->user_stack;
                frame->ss = 0x23;
                frame->eax = 0;
                frame->ebx = 0;
                frame->ecx = 0;
                frame->edx = 0;
                frame->ebp = 0;
                frame->esi = 0;
                frame->edi = 0;

                /* 同步更新 PCB */
                proc->context = *frame;
                proc->ticks_remaining = proc->time_slice;
            }
            ret = 0;
        } else if (syscall_num < SYSCALL_COUNT && syscall_table[syscall_num]) {
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
