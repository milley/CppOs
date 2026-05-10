/* shell.c - Shell 实现 */

#include "shell.h"
#include "keyboard.h"
#include "filesystem.h"
#include "string.h"
#include "syscall.h"
#include "process.h"

#define VIDEO_MEMORY 0xB8000
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0F
#define GREEN_ON_BLACK 0x0A
#define CYAN_ON_BLACK  0x0B

#define SHELL_BUFFER_SIZE 256

/* 外部函数 (kernel.c 中定义) */
extern void print_char(char c);
extern void print_string(const char *str);
extern void print_int(uint32_t value);
extern void clear_screen(void);

/* Shell 缓冲区 */
static char cmd_buffer[SHELL_BUFFER_SIZE];

/* 显示提示符 */
static void show_prompt(void) {
    print_string("MyOS> ");
}

/* 跳过空格 */
static char *skip_spaces(char *s) {
    while (*s == ' ') s++;
    return s;
}

/* 获取下一个参数 */
static char *get_arg(char *s, char *arg, int max_len) {
    s = skip_spaces(s);
    int i = 0;
    while (*s && *s != ' ' && i < max_len - 1) {
        arg[i++] = *s++;
    }
    arg[i] = '\0';
    return s;
}

/* 命令: help */
static void cmd_help(void) {
    print_string("Available commands:\n");
    print_string("  help          - Show this help\n");
    print_string("  clear         - Clear screen\n");
    print_string("  ls            - List files\n");
    print_string("  cat <file>    - Display file content\n");
    print_string("  touch <file>  - Create empty file\n");
    print_string("  rm <file>     - Delete file\n");
    print_string("  write <file> <text> - Write text to file\n");
    print_string("  echo <text>   - Print text\n");
    print_string("  ver           - Show OS version\n");
    print_string("  fork          - Test fork/wait syscalls\n");
    print_string("  pid           - Show current process ID\n");
}

/* 命令: clear */
static void cmd_clear(void) {
    clear_screen();
}

/* 命令: ls */
static void cmd_ls(void) {
    int count = 0;
    dir_entry_t *dir = fs_get_root_dir();

    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (dir[i].flags == 1) {
            print_string("  ");
            print_string(dir[i].name);
            print_string(" (");
            print_int(dir[i].file_size);
            print_string(" bytes)\n");
            count++;
        }
    }

    if (count == 0) {
        print_string("  (empty)\n");
    }

    print_string("Total: ");
    print_int(count);
    print_string(" files\n");
}

/* 命令: cat */
static void cmd_cat(char *filename) {
    if (filename[0] == '\0') {
        print_string("Usage: cat <filename>\n");
        return;
    }

    int fd = fs_open(filename, FS_MODE_READ);
    if (fd < 0) {
        print_string("File not found: ");
        print_string(filename);
        print_string("\n");
        return;
    }

    char buf[128];
    int n;
    while ((n = fs_read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        print_string(buf);
    }

    print_string("\n");
    fs_close(fd);
}

/* 命令: touch */
static void cmd_touch(char *filename) {
    if (filename[0] == '\0') {
        print_string("Usage: touch <filename>\n");
        return;
    }

    if (fs_create(filename) == 0) {
        print_string("Created: ");
        print_string(filename);
        print_string("\n");
    } else {
        print_string("Failed to create file\n");
    }
}

/* 命令: rm */
static void cmd_rm(char *filename) {
    if (filename[0] == '\0') {
        print_string("Usage: rm <filename>\n");
        return;
    }

    if (fs_delete(filename) == 0) {
        print_string("Deleted: ");
        print_string(filename);
        print_string("\n");
    } else {
        print_string("Failed to delete file\n");
    }
}

/* 命令: write */
static void cmd_write(char *args) {
    char filename[32];
    args = get_arg(args, filename, sizeof(filename));

    if (filename[0] == '\0') {
        print_string("Usage: write <filename> <text>\n");
        return;
    }

    args = skip_spaces(args);
    if (*args == '\0') {
        print_string("Usage: write <filename> <text>\n");
        return;
    }

    int fd = fs_open(filename, FS_MODE_WRITE);
    if (fd < 0) {
        /* 文件不存在，先创建 */
        fs_create(filename);
        fd = fs_open(filename, FS_MODE_WRITE);
        if (fd < 0) {
            print_string("Failed to open file\n");
            return;
        }
    }

    int len = strlen(args);
    int n = fs_write(fd, args, len);
    fs_close(fd);

    print_string("Wrote ");
    print_int(n);
    print_string(" bytes to ");
    print_string(filename);
    print_string("\n");
}

/* 命令: echo */
static void cmd_echo(char *text) {
    print_string(text);
    print_string("\n");
}

/* 命令: ver */
static void cmd_ver(void) {
    print_string("MyOS v1.0 - Simple Operating System\n");
    print_string("Built with love for learning OS development\n");
}

/* 命令: pid */
static void cmd_pid(void) {
    uint32_t pid = sys_getpid();
    print_string("Current PID: ");
    print_int(pid);
    print_string("\n");
}

/* 调试输出到屏幕顶部（第 0 行） */
static void debug_print(const char *msg, uint32_t val) {
    char *video = (char *)0xB8000;
    static int debug_col = 0;
    int offset = debug_col * 2;
    debug_col = (debug_col + 15) % 80;

    /* 清除该位置 */
    for (int i = 0; i < 15; i++) {
        video[offset + i * 2] = ' ';
        video[offset + i * 2 + 1] = 0x0C;
    }

    for (int i = 0; msg[i]; i++) {
        video[offset + i * 2] = msg[i];
        video[offset + i * 2 + 1] = 0x0C;  /* 红色 */
    }
    if (val != 0xFFFFFFFF) {
        int pos = strlen(msg);
        /* 显示数字 */
        if (val >= 10) {
            video[offset + pos * 2] = '0' + (val / 10);
            video[offset + pos * 2 + 1] = 0x0C;
            pos++;
        }
        video[offset + pos * 2] = '0' + (val % 10);
        video[offset + pos * 2 + 1] = 0x0C;
    }
}

/* 子进程测试函数 */
static void child_process_func(void) {
    debug_print("CHILD START", 0);
    uint32_t my_pid = sys_getpid();
    print_string("  [Child] PID = ");
    print_int(my_pid);
    print_string("\n");

    for (int i = 0; i < 3; i++) {
        print_string("  [Child] working... ");
        print_int(i + 1);
        print_string("\n");
        for (volatile int j = 0; j < 500000; j++);
    }

    debug_print("CHILD EXIT", 42);
    print_string("  [Child] exiting with status 42\n");
    sys_exit(42);
}

/* 命令: fork - 测试进程创建 */
static void cmd_fork(void) {
    debug_print("FORK START", 0);
    print_string("Testing process creation...\n");

    /* 使用 process_create_kernel 创建内核态子进程 */
    extern struct process* process_create_kernel(void (*entry)(void), uint32_t priority);
    struct process *child = process_create_kernel(child_process_func, PRIORITY_NORMAL);

    if (child == NULL) {
        debug_print("CREATE FAIL", 0);
        print_string("  Failed to create child process!\n");
        return;
    }

    debug_print("CREATE OK", child->pid);

    /* 设置父进程关系 */
    struct process *parent = process_get_current();
    if (parent != NULL) {
        child->parent = parent;
        child->next_sibling = parent->first_child;
        parent->first_child = child;
    }

    print_string("  [Parent] created child PID = ");
    print_int(child->pid);
    print_string("\n");

    debug_print("WAIT CALL", 0);

    /* 等待子进程 */
    int status = 0;
    int waited = sys_wait(&status);

    debug_print("WAIT RET", waited);

    print_string("  [Parent] child ");
    print_int(waited);
    print_string(" exited with status ");
    print_int(status);
    print_string("\n");
}

/* 执行命令 */
static void execute_command(char *cmd) {
    char command[32];

    cmd = skip_spaces(cmd);
    cmd = get_arg(cmd, command, sizeof(command));
    cmd = skip_spaces(cmd);

    if (command[0] == '\0') {
        return;
    }

    if (strcmp(command, "help") == 0) {
        cmd_help();
    } else if (strcmp(command, "clear") == 0) {
        cmd_clear();
    } else if (strcmp(command, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(command, "cat") == 0) {
        cmd_cat(cmd);
    } else if (strcmp(command, "touch") == 0) {
        cmd_touch(cmd);
    } else if (strcmp(command, "rm") == 0) {
        cmd_rm(cmd);
    } else if (strcmp(command, "write") == 0) {
        cmd_write(cmd);
    } else if (strcmp(command, "echo") == 0) {
        cmd_echo(cmd);
    } else if (strcmp(command, "ver") == 0) {
        cmd_ver();
    } else if (strcmp(command, "fork") == 0) {
        cmd_fork();
    } else if (strcmp(command, "pid") == 0) {
        cmd_pid();
    } else {
        print_string("Unknown command: ");
        print_string(command);
        print_string("\nType 'help' for available commands.\n");
    }
}

/* 读取命令行并回显 */
static void read_line(char *buffer, int max_len) {
    int i = 0;
    char c;
    char *video = (char *)VIDEO_MEMORY;

    while (i < max_len - 1) {
        c = keyboard_getchar();

        if (c == '\n') {
            /* 回车 */
            print_string("\n");
            buffer[i] = '\0';
            return;
        } else if (c == '\b') {
            /* 退格 */
            if (i > 0) {
                i--;
                /* 删除屏幕上的字符 */
                int offset = (i + 7) * 2;  /* 提示符后第 i 个位置 */
                video[offset] = ' ';
                video[offset + 1] = WHITE_ON_BLACK;
            }
        } else if (c >= 32 && c < 127) {
            /* 可打印字符 */
            buffer[i++] = c;
            print_char(c);
        }
    }

    buffer[i] = '\0';
    print_string("\n");
}

/* 初始化 Shell */
void shell_init(void) {
    keyboard_init();
}

/* 运行 Shell */
void shell_run(void) {
    print_string("\n=== MyOS Shell ===\n");
    print_string("Type 'help' for commands.\n\n");

    /* 调试：显示 shell 已启动 - 屏幕最顶部 */
    char *video = (char *)0xB8000;
    video[0] = 'S';
    video[1] = 0x0A;  /* 绿色 */
    video[2] = 'H';
    video[3] = 0x0A;
    video[4] = 'E';
    video[5] = 0x0A;
    video[6] = 'L';
    video[7] = 0x0A;
    video[8] = 'L';
    video[9] = 0x0A;

    while (1) {
        show_prompt();
        read_line(cmd_buffer, SHELL_BUFFER_SIZE);
        execute_command(cmd_buffer);
    }
}
