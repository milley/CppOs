/* allocator.h - 内存分配器头文件 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stdint.h>

/* 页大小 */
#define PAGE_SIZE 4096

/* 内存块头 */
struct memory_block {
    uint32_t size;                  /* 块大小（不包括头） */
    uint8_t  is_free;               /* 是否空闲 */
    uint8_t  padding[3];            /* 对齐填充 */
    struct memory_block *next;     /* 下一个块 */
    struct memory_block *prev;     /* 上一个块 */
} __attribute__((packed));

/* 函数声明 */

/* 初始化内存分配器 */
void allocator_init(uint32_t start, uint32_t size);

/* 分配内存 */
void* kmalloc(uint32_t size);

/* 释放内存 */
void kfree(void *ptr);

/* 分配一页 */
void* alloc_page(void);

/* 释放一页 */
void free_page(void *addr);

/* 获取内存使用统计 */
uint32_t get_total_memory(void);
uint32_t get_used_memory(void);
uint32_t get_free_memory(void);

#endif
