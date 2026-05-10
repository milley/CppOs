/* shell.c - Shell 实现 */

#include "shell.h"
#include "keyboard.h"
#include "filesystem.h"
#include "string.h"

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

    while (1) {
        show_prompt();
        read_line(cmd_buffer, SHELL_BUFFER_SIZE);
        execute_command(cmd_buffer);
    }
}
