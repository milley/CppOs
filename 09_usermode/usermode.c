/* usermode.c - 用户模式实现 */

#include "usermode.h"
#include "allocator.h"
#include "tss.h"

/* GDT 条目结构 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

/* GDT 指针 */
struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

/* GDT（全局描述符表） - 需要增加到 7 个条目（添加 TSS） */
struct gdt_entry gdt[7];  /* 改为外部可见，供 TSS 使用 */
static struct gdt_ptr gdt_pointer;

/* 设置 GDT 条目 */
static void set_gdt_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;

    gdt[num].access = access;
}

/* 初始化用户模式 GDT */
void usermode_init(void) {
    /* 设置 GDT 条目
     * 索引 0: 空描述符（必须）
     * 索引 1: 内核代码段 (0x08)
     * 索引 2: 内核数据段 (0x10)
     * 索引 3: 用户代码段 (0x18)
     * 索引 4: 用户数据段 (0x20)
     */

    /* 0: 空描述符 */
    set_gdt_entry(0, 0, 0, 0, 0);

    /* 1: 内核代码段 (Ring 0) */
    set_gdt_entry(1, 0, 0xFFFFFFFF,
        0x9A,  /* Present, Ring 0, Code, Executable, Readable */
        0xCF   /* 32-bit, 4KB granularity */
    );

    /* 2: 内核数据段 (Ring 0) */
    set_gdt_entry(2, 0, 0xFFFFFFFF,
        0x92,  /* Present, Ring 0, Data, Writable */
        0xCF
    );

    /* 3: 用户代码段 (Ring 3) */
    set_gdt_entry(3, 0, 0xFFFFFFFF,
        0xFA,  /* Present, Ring 3, Code, Executable, Readable */
        0xCF
    );

    /* 4: 用户数据段 (Ring 3) */
    set_gdt_entry(4, 0, 0xFFFFFFFF,
        0xF2,  /* Present, Ring 3, Data, Writable */
        0xCF
    );

    /* 加载 GDT */
    gdt_pointer.limit = sizeof(gdt) - 1;
    gdt_pointer.base = (uint32_t)&gdt;

    __asm__ volatile("lgdt %0" : : "m"(gdt_pointer));

    /* 初始化 TSS */
    tss_init();

    /* 设置内核栈（用于 Ring 3 -> Ring 0 切换） */
    tss_set_kernel_stack(0x90000);

    /* 重新加载段寄存器 */
    __asm__ volatile(
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : : "ax"
    );
}

/* 汇编函数声明 */
extern void jump_user_mode(uint32_t entry, uint32_t stack);

/* 切换到用户模式 */
void jump_to_usermode(user_entry_t entry, uint32_t stack_addr) {
    jump_user_mode((uint32_t)entry, stack_addr);
}

/* 创建用户进程 */
int create_user_process(const char *name, user_entry_t entry) {
    /* 分配用户栈 */
    void *stack = kmalloc(USER_STACK_SIZE);
    if (!stack) {
        return -1;
    }

    uint32_t stack_top = (uint32_t)stack + USER_STACK_SIZE;

    /* 切换到用户模式运行 */
    jump_to_usermode(entry, stack_top);

    /* 不会返回 */
    return 0;
}
