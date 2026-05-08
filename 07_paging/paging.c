/* paging.c - 分页机制实现 */

#include "paging.h"
#include "allocator.h"

/* 页目录（必须 4KB 对齐） */
static page_directory_t *page_directory = 0;

/* 页表数组（每个页目录项对应一个页表） */
static page_table_t *page_tables[1024];

/* 页目录物理地址 */
static uint32_t page_directory_phys = 0;

/* 端口 I/O */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* 读取 CR3 寄存器 */
static inline uint32_t read_cr3(void) {
    uint32_t value;
    __asm__ volatile("mov %%cr3, %0" : "=r"(value));
    return value;
}

/* 写入 CR3 寄存器 */
static inline void write_cr3(uint32_t value) {
    __asm__ volatile("mov %0, %%cr3" : : "r"(value));
}

/* 读取 CR0 寄存器 */
static inline uint32_t read_cr0(void) {
    uint32_t value;
    __asm__ volatile("mov %%cr0, %0" : "=r"(value));
    return value;
}

/* 写入 CR0 寄存器 */
static inline void write_cr0(uint32_t value) {
    __asm__ volatile("mov %0, %%cr0" : : "r"(value));
}

/* 刷新 TLB */
static inline void flush_tlb(void) {
    write_cr3(read_cr3());
}

/* 刷新单个页的 TLB */
static inline void invlpg(uint32_t addr) {
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

/* 初始化分页 */
void paging_init(void) {
    /* 分配页目录（必须 4KB 对齐） */
    page_directory = (page_directory_t *)kmalloc(sizeof(page_directory_t) + PAGE_SIZE);

    /* 确保 4KB 对齐 */
    uint32_t addr = (uint32_t)page_directory;
    addr = (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    page_directory = (page_directory_t *)addr;
    page_directory_phys = addr;

    /* 清空页目录 */
    for (int i = 0; i < 1024; i++) {
        page_directory->entries[i] = 0;
        page_tables[i] = 0;
    }

    /* 恒等映射前 4MB 内存（内核空间） */
    /* 这样虚拟地址 0x00000000 - 0x00400000 映射到相同的物理地址 */
    for (uint32_t addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }

    /* 加载页目录到 CR3 */
    write_cr3(page_directory_phys);

    /* 启用分页（设置 CR0 的 PG 位） */
    uint32_t cr0 = read_cr0();
    cr0 |= 0x80000000;  /* 设置第 31 位（PG 位） */
    write_cr0(cr0);
}

/* 映射虚拟页到物理帧 */
void map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    /* 计算页目录索引和页表索引 */
    uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;

    /* 检查页表是否存在 */
    if (!(page_directory->entries[page_dir_index] & PAGE_PRESENT)) {
        /* 分配新的页表 */
        page_table_t *new_table = (page_table_t *)kmalloc(sizeof(page_table_t) + PAGE_SIZE);

        /* 确保 4KB 对齐 */
        uint32_t table_addr = (uint32_t)new_table;
        table_addr = (table_addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        new_table = (page_table_t *)table_addr;

        /* 清空页表 */
        for (int i = 0; i < 1024; i++) {
            new_table->entries[i] = 0;
        }

        /* 在页目录中设置页表 */
        page_directory->entries[page_dir_index] = table_addr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        page_tables[page_dir_index] = new_table;
    }

    /* 获取页表 */
    uint32_t page_table_phys = page_directory->entries[page_dir_index] & 0xFFFFF000;
    page_table_t *page_table = (page_table_t *)page_table_phys;

    /* 设置页表项 */
    page_table->entries[page_table_index] = (physical_addr & 0xFFFFF000) | flags;

    /* 刷新 TLB */
    invlpg(virtual_addr);
}

/* 取消映射 */
void unmap_page(uint32_t virtual_addr) {
    uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;

    if (page_directory->entries[page_dir_index] & PAGE_PRESENT) {
        uint32_t page_table_phys = page_directory->entries[page_dir_index] & 0xFFFFF000;
        page_table_t *page_table = (page_table_t *)page_table_phys;

        page_table->entries[page_table_index] = 0;
        invlpg(virtual_addr);
    }
}

/* 获取物理地址 */
uint32_t get_physical_address(uint32_t virtual_addr) {
    uint32_t page_dir_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t page_table_index = (virtual_addr >> 12) & 0x3FF;
    uint32_t offset = virtual_addr & 0xFFF;

    if (!(page_directory->entries[page_dir_index] & PAGE_PRESENT)) {
        return 0;  /* 未映射 */
    }

    uint32_t page_table_phys = page_directory->entries[page_dir_index] & 0xFFFFF000;
    page_table_t *page_table = (page_table_t *)page_table_phys;

    if (!(page_table->entries[page_table_index] & PAGE_PRESENT)) {
        return 0;  /* 未映射 */
    }

    uint32_t page_phys = page_table->entries[page_table_index] & 0xFFFFF000;
    return page_phys + offset;
}

/* 获取当前页目录地址 */
uint32_t get_page_directory(void) {
    return page_directory_phys;
}

/* 分配一页物理内存并映射 */
void* alloc_page_at(uint32_t virtual_addr) {
    void *phys = kmalloc(PAGE_SIZE);
    if (!phys) {
        return 0;
    }

    map_page(virtual_addr, (uint32_t)phys, PAGE_PRESENT | PAGE_WRITABLE);
    return (void *)virtual_addr;
}
