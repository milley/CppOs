/* keyboard.h - 键盘驱动头文件 */

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* 初始化键盘 */
void keyboard_init(void);

/* 键盘中断处理 */
void keyboard_handler(void);

/* 读取键盘缓冲区 */
char keyboard_getchar(void);

/* 检查是否有按键 */
int keyboard_has_key(void);

#endif
