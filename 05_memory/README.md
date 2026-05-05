# 内存检测

使用 BIOS INT 0x15 EAX=0xE820 检测系统内存。

## 项目结构

```
05_memory/
├── bootloader.asm   # 引导程序（含内存检测）
├── kernel_entry.asm # 内核入口
├── kernel.c         # 主内核
├── memory.h         # 内存管理头文件
├── memory.c         # 内存检测实现
├── io.h             # 端口 I/O
├── Makefile
└── README.md
```

## 快速开始

```bash
make
make run
```

## 内存检测原理

### BIOS INT 0x15, EAX=0xE820

在实模式下调用：

```
输入:
  EAX = 0xE820 (功能号)
  EBX = 0      (从 0 开始)
  EDX = 'SMAP' (签名)
  ECX = 24     (缓冲区大小)
  ES:DI = 缓冲区地址

输出:
  EAX = 'SMAP' (签名确认)
  EBX = 下次调用值 (0=结束)
  ECX = 实际写入大小
```

### 内存映射条目结构（24字节）

```
偏移  大小  字段
0     8    基地址
8     8    长度
16    4    类型
20    4    ACPI 属性
```

### 内存类型

| 类型 | 说明 |
|------|------|
| 1 | 可用内存 |
| 2 | 保留内存 |
| 3 | ACPI 可回收 |
| 4 | ACPI NVS |
| 5 | 不可用 |

## 内存布局

```
0x5000         内存映射条目数 (4字节)
0x5004         内存映射条目数组
0x7C00         引导扇区
0x7E00         内核代码
0x90000        栈顶
```

## 关键代码

### 引导程序内存检测

```asm
detect_memory:
    mov edi, 0x5004       ; 存放地址
    xor ebx, ebx          ; 从 0 开始
    mov edx, 'SMAP'       ; 签名

.mem_loop:
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    jc .mem_done
    inc dword [0x5000]    ; 条目计数
    add edi, 24
    test ebx, ebx
    jnz .mem_loop
.mem_done:
    ret
```

### 内核读取内存映射

```c
uint32_t *entry_count_ptr = (uint32_t *)0x5000;
uint32_t entry_count = *entry_count_ptr;

uint8_t *entries = (uint8_t *)0x5004;
for (uint32_t i = 0; i < entry_count; i++) {
    uint8_t *entry = entries + (i * 24);
    uint32_t base = *(uint32_t *)(entry + 0);
    uint32_t length = *(uint32_t *)(entry + 8);
    uint32_t type = *(uint32_t *)(entry + 16);
    // ...
}
```

## 下一步

1. 位图内存分配器
2. kmalloc/kfree 实现
3. 分页机制
