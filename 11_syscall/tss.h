/* tss.h - 任务状态段头文件 */

#ifndef TSS_H
#define TSS_H

#include <stdint.h>

/* TSS 结构 */
struct tss_entry {
    uint16_t link;
    uint16_t link_pad;
    uint32_t esp0;
    uint16_t ss0;
    uint16_t ss0_pad;
    uint32_t esp1;
    uint16_t ss1;
    uint16_t ss1_pad;
    uint32_t esp2;
    uint16_t ss2;
    uint16_t ss2_pad;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint16_t es;
    uint16_t es_pad;
    uint16_t cs;
    uint16_t cs_pad;
    uint16_t ss;
    uint16_t ss_pad;
    uint16_t ds;
    uint16_t ds_pad;
    uint16_t fs;
    uint16_t fs_pad;
    uint16_t gs;
    uint16_t gs_pad;
    uint16_t ldtr;
    uint16_t ldtr_pad;
    uint16_t trap_pad;
    uint16_t iomap;
} __attribute__((packed));

void tss_init(void);
void tss_set_kernel_stack(uint32_t esp0);

#endif
