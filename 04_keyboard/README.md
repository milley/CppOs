# 键盘驱动

实现 PS/2 键盘驱动，支持用户输入。

## 项目结构

```
04_keyboard/
├── bootloader.asm    # 引导程序
├── kernel_entry.asm  # 内核入口
├── kernel.c          # 主内核（轮询方式）
├── idt.h/c           # 中断描述符表
├── idt.asm           # IDT 汇编入口
├── idt_load.asm      # 加载 IDT
├── isr.c             # 中断处理
├── isr.asm           # ISR 汇编入口
├── pic.h/c           # 可编程中断控制器
├── keyboard.h/c      # 键盘驱动
├── io.h              # 端口 I/O
├── Makefile
└── README.md
```

## 快速开始

```bash
make
make run
```

## 当前状态

- ✅ IDT 初始化
- ✅ PIC 初始化
- ✅ 键盘轮询读取
- ⚠️ 中断方式键盘输入待完善

## 键盘工作原理

### 轮询方式

```
while(1) {
    status = inb(0x64);     // 检查状态端口
    if (status & 0x01) {    // 有数据？
        scancode = inb(0x60);  // 读取扫描码
        // 转换为 ASCII
    }
}
```

### 中断方式

```
按键 → IRQ1 → IDT[33] → ISR → keyboard_handler()
```

## 键盘端口

| 端口 | 说明 |
|------|------|
| 0x60 | 数据端口（扫描码） |
| 0x64 | 状态端口 |

## 扫描码表

| 扫描码 | 按键 | ASCII |
|--------|------|-------|
| 0x02 | 1 | '1' |
| 0x10 | Q | 'q' |
| 0x1E | A | 'a' |
| 0x39 | Space | ' ' |

## 注意事项

Makefile 中链接顺序很重要：
- `kernel_entry.o` 必须放在第一位
- 确保 `_start` 在地址 0x7E00

## 下一步

1. 完善中断方式键盘输入
2. 命令行 Shell
3. 内存管理