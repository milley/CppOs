/* pic.h - 可编程中断控制器 */

#ifndef PIC_H
#define PIC_H

#include <stdint.h>

/* PIC 端口 */
#define PIC1_CMD  0x20    /* 主片命令端口 */
#define PIC1_DATA 0x21    /* 主片数据端口 */
#define PIC2_CMD  0xA0    /* 从片命令端口 */
#define PIC2_DATA 0xA1    /* 从片数据端口 */

/* PIC 命令 */
#define PIC_EOI   0x20    /* 中断结束命令 */

/* 初始化 PIC */
void pic_init(void);

/* 发送 EOI */
void pic_send_eoi(uint8_t irq);

/* 屏蔽/启用 IRQ */
void pic_mask_irq(uint8_t irq);
void pic_unmask_irq(uint8_t irq);

#endif
