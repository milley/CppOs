/* paging.h - 分页机制头文件 */

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/* 页大小 */
#define PAGE_SIZE 4096

/* 页目录项标志 */
#define PAGE_PRESENT    0x01
#define PAGE_WRITABLE   0x02
#define PAGE_USER       0x04
#define PAGE_ACCESSED   0x20
#define PAGE_DIRTY      0x40

/* 页目录和页表 */
typedef struct {
    uint32_t entries[1024];
} page_table_t;

typedef struct {
    uint32_t entries[1024];
} page_directory_t;

/* 地址空间结构 */
typedef struct {
    page_directory_t *directory;
    uint32_t directory_phys;
} address_space_t;

/* 函数声明 */
void paging_init(void);
address_space_t* address_space_create(void);
void address_space_destroy(address_space_t *space);
void address_space_switch(address_space_t *space);
void map_page(address_space_t *space, uint32_t virt, uint32_t phys, uint32_t flags);
void* alloc_and_map_page(address_space_t *space, uint32_t virt, uint32_t flags);
void map_kernel_space(address_space_t *space);
uint32_t alloc_physical_page(void);
void free_physical_page(uint32_t addr);
uint32_t get_free_physical_pages(void);

#endif
