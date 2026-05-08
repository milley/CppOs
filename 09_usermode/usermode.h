/* usermode.h - 用户模式头文件 */

#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>

/* 特权级定义 */
#define RING_0  0   /* 内核态 */
#define RING_1  1   /* 系统服务（很少使用） */
#define RING_2  2   /* 系统服务（很少使用） */
#define RING_3  3   /* 用户态 */

/* 段选择子 */
#define KERNEL_CS  0x08   /* 内核代码段 */
#define KERNEL_DS  0x10   /* 内核数据段 */
#define USER_CS    0x1B   /* 用户代码段 (0x18 | RING_3) */
#define USER_DS    0x23   /* 用户数据段 (0x20 | RING_3) */

/* 用户栈大小 */
#define USER_STACK_SIZE 4096

/* 用户进程入口点类型 */
typedef void (*user_entry_t)(void);

/* 函数声明 */

/* 初始化用户模式（设置 GDT） */
void usermode_init(void);

/* 切换到用户模式运行程序 */
void jump_to_usermode(user_entry_t entry, uint32_t stack_addr);

/* 创建用户进程 */
int create_user_process(const char *name, user_entry_t entry);

/* 从用户态进入内核态（系统调用入口） */
void syscall_entry(void);

#endif
