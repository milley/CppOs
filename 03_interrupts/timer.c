/* timer.c - 定时器驱动实现 */

#include "timer.h"
#include "idt.h"
#include "pic.h"
#include "io.h"

/* 时钟滴答计数 */
static uint32_t ticks = 0;

/* 定时器中断处理 */
void timer_handler(void) {
    ticks++;
}

/* 获取滴答数 */
uint32_t timer_get_ticks(void) {
    return ticks;
}

/* 初始化定时器 */
void timer_init(uint32_t frequency) {
    /* 计算分频值 */
    uint32_t divisor = 1193180 / frequency;

    /* 发送命令到 PIT */
    outb(0x43, 0x36);           /* 通道0，模式3，先低后高 */

    /* 设置分频值 */
    outb(0x40, divisor & 0xFF);         /* 低字节 */
    outb(0x40, (divisor >> 8) & 0xFF);  /* 高字节 */

    /* 启用 IRQ0（定时器） */
    pic_unmask_irq(0);
}
