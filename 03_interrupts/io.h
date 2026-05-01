/* io.h - 端口 I/O 函数 */

#ifndef IO_H
#define IO_H

#include <stdint.h>

/* 端口输出 */
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/* 端口输入 */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif
