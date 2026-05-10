/* usermode.h - 用户模式头文件 */

#ifndef USERMODE_H
#define USERMODE_H

#include <stdint.h>

/* USER_STACK_SIZE 在 process.h 中定义 */

typedef void (*user_entry_t)(void);

void usermode_init(void);
void jump_to_usermode(user_entry_t entry, uint32_t stack_addr);

#endif
