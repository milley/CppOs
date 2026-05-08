/* tss.h - 任务状态段头文件 */

#ifndef TSS_H
#define TSS_H

#include <stdint.h>

/* TSS 结构 */
struct tss_entry {
    uint16_t link;      /* 16位：上一个任务的链接 */
    uint16_t link_pad;
    uint32_t esp0;      /* 32位：Ring 0 栈指针 */
    uint16_t ss0;       /* 16位：Ring 0 栈段 */
    uint16_t ss0_pad;
    uint32_t esp1;      /* Ring 1（未使用） */
    uint16_t ss1;
    uint16_t ss1_pad;
    uint32_t esp2;      /* Ring 2（未使用） */
    uint16_t ss2;
    uint16_t ss2_pad;
    uint32_t cr3;       /* 页目录基址 */
    uint32_t eip;       /* 指令指针 */
    uint32_t eflags;    /* 标志寄存器 */
    uint32_t eax;       /* 通用寄存器 */
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;       /* 栈指针 */
    uint32_t ebp;       /* 基址指针 */
    uint32_t esi;       /* 源索引 */
    uint32_t edi;       /* 目标索引 */
    uint16_t es;        /* 段寄存器 */
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
    uint16_t ldtr;      /* 局部描述符表 */
    uint16_t ldtr_pad;
    uint16_t trap_pad;
    uint16_t iomap;     /* I/O 位图基址 */
} __attribute__((packed));

/* 初始化 TSS */
void tss_init(void);

/* 设置 Ring 0 栈 */
void tss_set_kernel_stack(uint32_t esp0);

#endif
