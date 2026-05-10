/* paging.c - 分页机制实现（简化版） */

#include <stddef.h>
#include "paging.h"
#include "allocator.h"

/* 内核页目录 */
static page_directory_t *kernel_directory = NULL;
static uint32_t kernel_directory_phys = 0;

/* 当前地址空间 */
static address_space_t *current_space = NULL;

/* 物理页位图 */
#define MAX_PHYSICAL_PAGES 16384
static uint8_t physical_page_bitmap[MAX_PHYSICAL_PAGES / 8];
static uint32_t total_physical_pages = 0;
static uint32_t first_free_page = 0;

#define PHYSICAL_MEMORY_START 0x100000

/* CR 寄存器操作 */
static inline uint32_t read_cr3(void) {
    uint32_t v;
    __asm__ volatile("mov %%cr3, %0" : "=r"(v));
    return v;
}

static inline void write_cr3(uint32_t v) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(v));
}

static inline uint32_t read_cr0(void) {
    uint32_t v;
    __asm__ volatile("mov %%cr0, %0" : "=r"(v));
    return v;
}

static inline void write_cr0(uint32_t v) {
    __asm__ volatile("mov %0, %%cr0" : : "r"(v));
}

/* 初始化物理页位图 */
static void init_physical_page_bitmap(void) {
    total_physical_pages = MAX_PHYSICAL_PAGES;

    for (uint32_t i = 0; i < sizeof(physical_page_bitmap); i++) {
        physical_page_bitmap[i] = 0;
    }

    /* 标记前 8MB 已使用 */
    for (uint32_t i = 0; i < 0x700000 / PAGE_SIZE; i++) {
        uint32_t byte = i / 8;
        uint32_t bit = i % 8;
        physical_page_bitmap[byte] |= (1 << bit);
    }

    first_free_page = 0x700000 / PAGE_SIZE;
}

/* 分配物理页 */
uint32_t alloc_physical_page(void) {
    for (uint32_t i = first_free_page; i < total_physical_pages; i++) {
        uint32_t byte = i / 8;
        uint32_t bit = i % 8;
        if (!(physical_page_bitmap[byte] & (1 << bit))) {
            physical_page_bitmap[byte] |= (1 << bit);
            first_free_page = i + 1;
            return PHYSICAL_MEMORY_START + i * PAGE_SIZE;
        }
    }
    return 0;
}

/* 释放物理页 */
void free_physical_page(uint32_t addr) {
    if (addr < PHYSICAL_MEMORY_START) return;
    uint32_t i = (addr - PHYSICAL_MEMORY_START) / PAGE_SIZE;
    if (i >= total_physical_pages) return;
    physical_page_bitmap[i / 8] &= ~(1 << (i % 8));
    if (i < first_free_page) first_free_page = i;
}

/* 获取空闲页数 */
uint32_t get_free_physical_pages(void) {
    uint32_t count = 0;
    for (uint32_t i = 0; i < total_physical_pages; i++) {
        if (!(physical_page_bitmap[i / 8] & (1 << (i % 8)))) count++;
    }
    return count;
}

/* 映射页面（简化版：记录但不实际映射） */
void map_page(address_space_t *space, uint32_t virt, uint32_t phys, uint32_t flags) {
    if (!space || !space->directory) return;
    (void)virt;
    (void)phys;
    (void)flags;
    /* 简化版：不实际修改页表 */
}

/* 分配并映射页面（简化版） */
void* alloc_and_map_page(address_space_t *space, uint32_t virt, uint32_t flags) {
    uint32_t phys = alloc_physical_page();
    if (phys == 0) return NULL;
    map_page(space, virt, phys, flags);
    /* 返回物理地址（平坦内存模型下直接可用） */
    return (void*)phys;
}

/* 映射内核空间 */
void map_kernel_space(address_space_t *space) {
    (void)space;
    /* 简化版：无需操作 */
}

/* 创建地址空间 */
address_space_t* address_space_create(void) {
    address_space_t *space = (address_space_t*)kmalloc(sizeof(address_space_t));
    if (!space) return NULL;

    space->directory = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!space->directory) {
        kfree(space);
        return NULL;
    }

    space->directory_phys = (uint32_t)space->directory;

    /* 清空页目录 */
    for (int i = 0; i < 1024; i++) {
        space->directory->entries[i] = 0;
    }

    return space;
}

/* 销毁地址空间 */
void address_space_destroy(address_space_t *space) {
    if (!space) return;
    if (space->directory) kfree(space->directory);
    kfree(space);
}

/* 切换地址空间 */
void address_space_switch(address_space_t *space) {
    if (!space) return;
    current_space = space;
    /* 简化版：不实际切换 CR3（保持平坦内存模型） */
}

/* 初始化分页（简化版：仅初始化物理页管理） */
void paging_init(void) {
    init_physical_page_bitmap();

    /* 创建内核地址空间结构 */
    kernel_directory = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    kernel_directory_phys = (uint32_t)kernel_directory;

    for (int i = 0; i < 1024; i++) {
        kernel_directory->entries[i] = 0;
    }

    current_space = (address_space_t*)kmalloc(sizeof(address_space_t));
    current_space->directory = kernel_directory;
    current_space->directory_phys = kernel_directory_phys;

    /* 注意：不启用分页，保持平坦内存模型 */
    /* 这样所有进程共享同一地址空间，但物理内存独立分配 */
}
