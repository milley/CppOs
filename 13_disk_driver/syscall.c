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

/* 外部函数声明 */
extern void print_string(const char *str);
extern void print_int(uint32_t value);

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
            process_unregister(child);
            kfree(child);

            return child_pid;
        }
        prev = child;
        child = child->next_sibling;
    }

    /* 没有僵尸子进程，返回 -2 表示需要阻塞等待 */
    return (uint32_t)-2;
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

            if (parent == NULL) {
                ret = (uint32_t)-1;
            } else {
                /* 分配 PID */
                uint32_t pid = 0;
                for (uint32_t i = 1; i < MAX_PROCESSES; i++) {
                    if (process_find_by_pid(i) == NULL) {
                        pid = i;
                        break;
                    }
                }

                if (pid == 0) {
                    ret = (uint32_t)-1;
                } else {
                    /* 分配 PCB */
                    struct process *child = (struct process *)kmalloc(sizeof(struct process));
                    if (child == NULL) {
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

                        /* 父进程返回子进程 PID */
                        ret = pid;
                    }
                }
            }
        } else if (syscall_num == SYS_EXEC) {
            /* exec - 从文件系统加载并执行程序 */
            const char *filename = (const char *)arg1;

            if (filename == NULL) {
                ret = (uint32_t)-1;
            } else {
                /* 打开文件 */
                int fd = fs_open(filename, FS_MODE_READ);
                if (fd < 0) {
                    ret = (uint32_t)-1;  /* 文件不存在 */
                } else {
                    /* 获取文件大小 */
                    int32_t file_size = fs_size(fd);
                    if (file_size <= 0 || file_size > FS_MAX_FILESIZE) {
                        fs_close(fd);
                        ret = (uint32_t)-1;
                    } else {
                        /* 分配内存加载程序（使用固定地址 0x100000） */
                        uint8_t *program_base = (uint8_t *)0x100000;

                        /* 清零程序区域 */
                        for (int i = 0; i < file_size + 4096; i++) {
                            program_base[i] = 0;
                        }

                        /* 读取文件内容 */
                        int bytes_read = fs_read(fd, program_base, file_size);
                        fs_close(fd);

                        if (bytes_read != file_size) {
                            ret = (uint32_t)-1;
                        } else {
                            /* 检查程序类型 */
                            /* SEX1 格式: 魔数 "SEX1" + entry_offset(4) + code_size(4) + code */
                            if (file_size >= 12 &&
                                program_base[0] == 'S' &&
                                program_base[1] == 'E' &&
                                program_base[2] == 'X' &&
                                program_base[3] == '1') {

                                /* 解析头部 */
                                uint32_t entry_offset = *(uint32_t*)(program_base + 4);
                                uint32_t code_size = *(uint32_t*)(program_base + 8);

                                /* 验证 */
                                if (entry_offset + code_size > (uint32_t)file_size - 12) {
                                    ret = (uint32_t)-1;
                                } else {
                                    /* 代码从头部后开始 */
                                    uint8_t *code_start = program_base + 12;

                                    /* 移动代码到执行位置 */
                                    for (uint32_t i = 0; i < code_size; i++) {
                                        program_base[i] = code_start[i];
                                    }

                                    /* 设置进程上下文执行代码 */
                                    struct process *proc = process_get_current();
                                    if (proc != NULL) {
                                        /* 设置 EIP 指向程序入口 */
                                        frame->eip = (uint32_t)program_base + entry_offset;
                                        frame->cs = 0x08;        /* 内核代码段 */
                                        frame->eflags = 0x202;
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

                                        ret = 0;
                                    } else {
                                        ret = (uint32_t)-1;
                                    }
                                }
                            } else {
                                /* 不是 SEX1 格式，尝试解释执行 */
                                /* 如果以 "HELLO" 开头，执行内置的 hello 程序 */
                                if (file_size >= 5 &&
                                    program_base[0] == 'H' &&
                                    program_base[1] == 'E' &&
                                    program_base[2] == 'L' &&
                                    program_base[3] == 'L' &&
                                    program_base[4] == 'O') {

                                    /* 输出到屏幕固定位置 */
                                    char *video = (char *)0xB8000;
                                    int offset = 20 * 160;
                                    for (int i = 0; i < 80; i++) {
                                        video[offset + i * 2] = ' ';
                                        video[offset + i * 2 + 1] = 0x0A;
                                    }
                                    const char *msg = "EXEC: Hello from program!";
                                    for (int i = 0; msg[i]; i++) {
                                        video[offset + i * 2] = msg[i];
                                        video[offset + i * 2 + 1] = 0x0A;
                                    }
                                    ret = 0;
                                } else {
                                    /* 未知格式 */
                                    ret = (uint32_t)-1;
                                }
                            }
                        }
                    }
                }
            }
        } else if (syscall_num < SYSCALL_COUNT && syscall_table[syscall_num]) {
            ret = syscall_table[syscall_num](arg1, arg2, arg3);

            /* 特殊处理：wait 返回 -2 表示需要阻塞等待 */
            if (syscall_num == SYS_WAIT && ret == (uint32_t)-2) {
                struct process *parent = process_get_current();
                if (parent != NULL) {
                    /* 保存当前上下文到父进程 */
                    parent->context = *frame;
                    /* 保存 status 参数，以便恢复时使用 */
                    parent->context.ebx = arg1;  /* status 指针保存在 ebx */
                    /* 设置 eax = SYS_WAIT，以便恢复时重新调用 */
                    parent->context.eax = SYS_WAIT;
                    /* 阻塞父进程 */
                    process_block(parent);
                    /* 调度到其他进程（子进程） */
                    schedule_from_interrupt(frame);
                    /* 不会返回到这里 */
                    return;
                }
            }
        }

        /* 返回值放在 eax */
    frame->eax = ret;

        /* 检查是否需要调度（当前进程被阻塞或时间片用完） */
        struct process *current = process_get_current();
        if (current != NULL && current->ticks_remaining == 0) {
            /* 需要调度，调用 schedule_from_interrupt 切换进程 */
            /* 注意：frame 会被新进程的上下文覆盖 */
            schedule_from_interrupt(frame);
            /* 切换后不会返回这里，iret 会返回到新进程 */
        }
}

/* 初始化系统调用 */
void syscall_init(void) {
    /* 注册系统调用处理函数到 IDT 0x80 */
    idt_register_handler(0x80, syscall_handler);
}
