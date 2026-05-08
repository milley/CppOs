/* tss.c - 任务状态段实现 */

#include "tss.h"

/* TSS 条目 */
static struct tss_entry tss;

/* GDT 条目结构 */
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

/* 外部 GDT（在 usermode.c 中定义） */
extern struct gdt_entry gdt[6];

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

/* 初始化 TSS */
void tss_init(void) {
    /* 清空 TSS */
    for (int i = 0; i < sizeof(struct tss_entry); i++) {
        ((char *)&tss)[i] = 0;
    }

    /* 设置 Ring 0 栈段 */
    tss.ss0 = 0x10;  /* 内核数据段 */

    /* 设置 I/O 位图（禁用所有 I/O） */
    tss.iomap = sizeof(struct tss_entry);

    /* 在 GDT 中添加 TSS 描述符（索引 5，选择子 0x28） */
    set_gdt_entry(5, (uint32_t)&tss, sizeof(struct tss_entry) - 1,
        0x89,  /* Present, TSS 描述符类型 */
        0x00   /* 字节粒度 */
    );

    /* 加载 TR（任务寄存器） */
    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}

/* 设置 Ring 0 栈 */
void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}
