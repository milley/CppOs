/* memory.h - 内存管理头文件 */

#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

/* 内存信息结构 */
struct memory_map_entry {
    uint64_t base_address;      /* 基地址 */
    uint64_t length;            /* 长度 */
    uint32_t type;              /* 类型 */
    uint32_t acpi;              /* ACPI 属性 */
} __attribute__((packed));

/* 内存类型 */
#define MEMORY_TYPE_USABLE      1   /* 可用内存 */
#define MEMORY_TYPE_RESERVED    2   /* 保留内存 */
#define MEMORY_TYPE_ACPI        3   /* ACPI 可回收 */
#define MEMORY_TYPE_NVS         4   /* ACPI NVS */
#define MEMORY_TYPE_UNUSABLE    5   /* 不可用 */

/* 内存统计 */
struct memory_info {
    uint32_t total_memory;      /* 总内存 (KB) */
    uint32_t usable_memory;     /* 可用内存 (KB) */
    uint32_t entry_count;       /* 内存条目数 */
};

/* 函数声明 */
void memory_detect(void);
void memory_print_info(void);
uint32_t memory_get_total(void);
uint32_t memory_get_usable(void);

#endif
