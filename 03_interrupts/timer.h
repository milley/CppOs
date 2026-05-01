/* timer.h - 定时器驱动 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/* 初始化定时器 */
void timer_init(uint32_t frequency);

/* 定时器中断处理 */
void timer_handler(void);

/* 获取时钟滴答数 */
uint32_t timer_get_ticks(void);

#endif
