/* paging.h - 分页机制头文件 */

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* 页大小 */
#define PAGE_SIZE 4096

/* 页目录项标志 */
#define PAGE_PRESENT    0x01    /* 页在内存中 */
#define PAGE_WRITABLE   0x02    /* 可写 */
#define PAGE_USER       0x04    /* 用户态可访问 */
#define PAGE_WRITETHROUGH 0x08  /* 写透 */
#define PAGE_CACHE_DISABLE 0x10 /* 禁用缓存 */
#define PAGE_ACCESSED   0x20    /* 已访问 */
#define PAGE_DIRTY      0x40    /* 已修改（页表项） */
#define PAGE_4MB        0x80    /* 4MB 页（页目录项） */
#define PAGE_GLOBAL     0x100   /* 全局页 */

/* 页目录和页表结构 */
typedef struct {
    uint32_t entries[1024];
} page_table_t;

typedef struct {
    uint32_t entries[1024];
} page_directory_t;

/* 页目录项结构（32位） */
/*
 * 位 0: Present
 * 位 1: Read/Write
 * 位 2: User/Supervisor
 * 位 3: Write Through
 * 位 4: Cache Disable
 * 位 5: Accessed
 * 位 6: 0 (Reserved)
 * 位 7: Page Size (0 = 4KB)
 * 位 8-11: 0 (Reserved)
 * 位 12-31: 页表物理地址（4KB对齐）
 */

/* 函数声明 */

/* 初始化分页 */
void paging_init(void);

/* 获取当前页目录地址 */
uint32_t get_page_directory(void);

/* 映射虚拟页到物理帧 */
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

/* 取消映射 */
void unmap_page(uint32_t virtual_addr);

/* 获取物理地址 */
uint32_t get_physical_address(uint32_t virtual_addr);

/* 分配一页物理内存并映射 */
void* alloc_page_at(uint32_t virtual_addr);

/* 页错误处理 */
void page_fault_handler(void);

#endif
