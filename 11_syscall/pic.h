/* pic.h - 可编程中断控制器头文件 */

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

/* PIC 端口 */
#define PIC1_CMD  0x20    /* 主 PIC 命令端口 */
#define PIC1_DATA 0x21    /* 主 PIC 数据端口 */
#define PIC2_CMD  0xA0    /* 从 PIC 命令端口 */
#define PIC2_DATA 0xA1    /* 从 PIC 数据端口 */

/* PIC 命令 */
#define PIC_EOI   0x20    /* 中断结束命令 */

/* 初始化 PIC */
void pic_init(void);

/* 发送 EOI（中断结束） */
void pic_send_eoi(uint8_t irq);

/* 屏蔽 IRQ */
void pic_mask_irq(uint8_t irq);

/* 取消屏蔽 IRQ */
void pic_unmask_irq(uint8_t irq);

#endif
