/* keyboard.h - 键盘驱动 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* 初始化键盘驱动 */
void keyboard_init(void);

/* 检查是否有按键可用 */
int keyboard_has_char(void);

/* 读取一个字符 (阻塞) */
char keyboard_getchar(void);

/* 读取一行 (阻塞，遇到回车返回) */
int keyboard_getline(char *buffer, int max_len);

/* 键盘中断处理函数 */
void keyboard_irq_handler(void);

#endif
