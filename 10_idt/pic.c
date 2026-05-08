/* pic.c - 可编程中断控制器实现 */

#include "pic.h"

/* 端口 I/O */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* 初始化 PIC */
void pic_init(void) {
    /* ICW1: 初始化命令 */
    outb(PIC1_CMD, 0x11);  /* 主 PIC：级联模式，需要 ICW4 */
    outb(PIC2_CMD, 0x11);  /* 从 PIC */

    /* ICW2: 设置中断向量基址 */
    outb(PIC1_DATA, 0x20); /* 主 PIC：IRQ 0-7 -> 中断 32-39 */
    outb(PIC2_DATA, 0x28); /* 从 PIC：IRQ 8-15 -> 中断 40-47 */

    /* ICW3: 设置级联关系 */
    outb(PIC1_DATA, 0x04); /* 主 PIC：IR2 连接从 PIC */
    outb(PIC2_DATA, 0x02); /* 从 PIC：连接到主 PIC 的 IR2 */

    /* ICW4: 设置工作模式 */
    outb(PIC1_DATA, 0x01); /* 8086 模式 */
    outb(PIC2_DATA, 0x01);

    /* 屏蔽所有 IRQ（除了级联 IRQ2） */
    outb(PIC1_DATA, 0xFB); /* 11111011 - 只允许 IRQ2（级联） */
    outb(PIC2_DATA, 0xFF); /* 11111111 - 屏蔽所有 */
}

/* 发送 EOI */
void pic_send_eoi(uint8_t irq) {
    /* 如果是从 PIC 的 IRQ，需要发送 EOI 给两个 PIC */
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

/* 屏蔽 IRQ */
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

/* 取消屏蔽 IRQ */
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
