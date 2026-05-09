/* tss.c - 任务状态段实现 */

#include "tss.h"

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

/* 外部 GDT */
extern struct gdt_entry gdt[];

static void set_gdt_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void tss_init(void) {
    for (int i = 0; i < sizeof(struct tss_entry); i++) {
        ((char *)&tss)[i] = 0;
    }
    tss.ss0 = 0x10;
    tss.iomap = sizeof(struct tss_entry);

    set_gdt_entry(5, (uint32_t)&tss, sizeof(struct tss_entry) - 1, 0x89, 0x00);

    __asm__ volatile("ltr %0" : : "r"((uint16_t)0x28));
}

void tss_set_kernel_stack(uint32_t esp0) {
    tss.esp0 = esp0;
}
