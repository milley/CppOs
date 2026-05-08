/* allocator.c - 内存分配器实现 */

#include "allocator.h"

/* 内存块头大小 */
#define BLOCK_HEADER_SIZE sizeof(struct memory_block)

/* 最小块大小 */
#define MIN_BLOCK_SIZE 16

/* 堆起始地址和大小 */
static uint32_t heap_start = 0;
static uint32_t heap_size = 0;

/* 内存统计 */
static uint32_t total_memory = 0;
static uint32_t used_memory = 0;

/* 第一个内存块 */
static struct memory_block *first_block = 0;

/* 初始化内存分配器 */
void allocator_init(uint32_t start, uint32_t size) {
    heap_start = start;
    heap_size = size;
    total_memory = size;
    used_memory = 0;

    /* 创建第一个大块 */
    first_block = (struct memory_block *)start;
    first_block->size = size - BLOCK_HEADER_SIZE;
    first_block->is_free = 1;
    first_block->next = 0;
    first_block->prev = 0;
}

/* 分割内存块 */
static void split_block(struct memory_block *block, uint32_t size) {
    /* 检查是否可以分割 */
    if (block->size < size + BLOCK_HEADER_SIZE + MIN_BLOCK_SIZE) {
        return;
    }

    /* 创建新块 */
    struct memory_block *new_block = (struct memory_block *)((uint8_t *)block + BLOCK_HEADER_SIZE + size);
    new_block->size = block->size - size - BLOCK_HEADER_SIZE;
    new_block->is_free = 1;
    new_block->next = block->next;
    new_block->prev = block;

    /* 更新原块 */
    block->size = size;
    block->next = new_block;

    /* 更新下一个块的 prev */
    if (new_block->next) {
        new_block->next->prev = new_block;
    }
}

/* 合并空闲块 */
static void merge_blocks(struct memory_block *block) {
    /* 合并后面的块 */
    if (block->next && block->next->is_free) {
        block->size += BLOCK_HEADER_SIZE + block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }

    /* 合并前面的块 */
    if (block->prev && block->prev->is_free) {
        block->prev->size += BLOCK_HEADER_SIZE + block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

/* 分配内存 */
void* kmalloc(uint32_t size) {
    if (size == 0) {
        return 0;
    }

    /* 对齐到 4 字节 */
    size = (size + 3) & ~3;

    /* 查找合适的空闲块 */
    struct memory_block *block = first_block;
    while (block) {
        if (block->is_free && block->size >= size) {
            /* 找到合适的块 */

            /* 尝试分割 */
            split_block(block, size);

            /* 标记为已使用 */
            block->is_free = 0;
            used_memory += block->size + BLOCK_HEADER_SIZE;

            /* 返回数据区域 */
            return (void *)((uint8_t *)block + BLOCK_HEADER_SIZE);
        }
        block = block->next;
    }

    /* 没有找到合适的块 */
    return 0;
}

/* 释放内存 */
void kfree(void *ptr) {
    if (ptr == 0) {
        return;
    }

    /* 获取块头 */
    struct memory_block *block = (struct memory_block *)((uint8_t *)ptr - BLOCK_HEADER_SIZE);

    /* 检查是否在堆范围内 */
    if ((uint32_t)block < heap_start || (uint32_t)block >= heap_start + heap_size) {
        return;
    }

    /* 标记为空闲 */
    if (!block->is_free) {
        used_memory -= block->size + BLOCK_HEADER_SIZE;
    }
    block->is_free = 1;

    /* 合并相邻空闲块 */
    merge_blocks(block);
}

/* 分配一页 */
void* alloc_page(void) {
    return kmalloc(PAGE_SIZE);
}

/* 释放一页 */
void free_page(void *addr) {
    kfree(addr);
}

/* 获取统计信息 */
uint32_t get_total_memory(void) {
    return total_memory;
}

uint32_t get_used_memory(void) {
    return used_memory;
}

uint32_t get_free_memory(void) {
    return total_memory - used_memory;
}
