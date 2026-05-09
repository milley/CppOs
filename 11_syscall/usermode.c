/* usermode.c - 用户模式实现 */

#include "usermode.h"
#include "allocator.h"
#include "tss.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct gdt_entry gdt[7];
static struct gdt_ptr gdt_pointer;

extern void jump_user_mode(uint32_t entry, uint32_t stack);

static void set_gdt_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access = access;
}

void usermode_init(void) {
    set_gdt_entry(0, 0, 0, 0, 0);
    set_gdt_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);
    set_gdt_entry(2, 0, 0xFFFFFFFF, 0x92, 0xCF);
    set_gdt_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);
    set_gdt_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

    gdt_pointer.limit = sizeof(gdt) - 1;
    gdt_pointer.base = (uint32_t)&gdt;

    __asm__ volatile("lgdt %0" : : "m"(gdt_pointer));

    tss_init();
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

    /* 重新加载 IDT（确保在新 GDT 下有效） */
    extern void idt_reload(void);
    idt_reload();
}

void jump_to_usermode(user_entry_t entry, uint32_t stack_addr) {
    jump_user_mode((uint32_t)entry, stack_addr);
}
