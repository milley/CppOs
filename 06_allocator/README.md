# 内存分配器

实现简单的内核内存分配器，支持 kmalloc/kfree。

## 项目结构

```
06_allocator/
├── bootloader.asm   # 引导程序
├── kernel_entry.asm # 内核入口
├── kernel.c         # 主内核（测试代码）
├── allocator.h      # 分配器头文件
├── allocator.c      # 分配器实现
├── Makefile
└── README.md
```

## 快速开始

```bash
make
make run
```

## 功能

- `kmalloc(size)` - 分配指定大小的内存
- `kfree(ptr)` - 释放内存
- `alloc_page()` - 分配一页（4KB）
- `free_page(addr)` - 释放一页

## 实现原理

### 内存块结构

```c
struct memory_block {
    uint32_t size;              // 块大小
    uint8_t  is_free;           // 是否空闲
    struct memory_block *next;  // 下一个块
    struct memory_block *prev;  // 上一个块
};
```

### 分配算法

```
1. 遍历空闲块链表
2. 找到第一个足够大的块 (First Fit)
3. 如果块太大，分割它
4. 标记为已使用
5. 返回数据区域地址
```

### 释放算法

```
1. 获取块头
2. 标记为空闲
3. 合并相邻的空闲块
```

## 内存布局

```
0x00100000    堆起始地址 (1MB)
0x00200000    堆结束地址 (2MB)
0x00007E00    内核代码
0x00090000    栈顶
```

## 运行效果

```
========================================
   MyOS - Memory Allocator Demo
========================================

Memory Status:
  Total: 1048576 bytes
  Used:  0 bytes
  Free:  1048576 bytes

Test 1: Allocate 100 bytes
  ptr1 = 0x00100010
...
```

## 下一步

1. 分页机制
2. 虚拟内存
3. 进程管理
