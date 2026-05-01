/* pic.c - 可编程中断控制器实现 */

#include "pic.h"
#include "io.h"

/* 初始化 PIC */
void pic_init(void) {
    /* ICW1: 初始化命令 */
    outb(PIC1_CMD, 0x11);   /* 初始化 + 需要 ICW4 */
    outb(PIC2_CMD, 0x11);

    /* ICW2: 设置中断向量偏移 */
    outb(PIC1_DATA, 0x20);  /* 主片: IRQ 0-7 -> 中断 32-39 */
    outb(PIC2_DATA, 0x28);  /* 从片: IRQ 8-15 -> 中断 40-47 */

    /* ICW3: 设置级联 */
    outb(PIC1_DATA, 0x04);  /* 主片 IR2 连接从片 */
    outb(PIC2_DATA, 0x02);  /* 从片连接到主片 IR2 */

    /* ICW4: 8086 模式 */
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /* 屏蔽所有中断 */
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

/* 发送中断结束命令 */
void pic_send_eoi(uint8_t irq) {
    outb(PIC1_CMD, PIC_EOI);
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
}

/* 屏蔽指定 IRQ */
void pic_mask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);
    outb(port, value);
}

/* 启用指定 IRQ */
void pic_unmask_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);
    outb(port, value);
}
