# 第一个引导程序

在屏幕上显示 "Hello, OS!" 的最简单引导程序。

## 项目结构

```
01_bootloader/
├── bootloader.asm   # 引导程序源码（汇编）
├── Makefile         # 构建脚本
├── Dockerfile       # 开发环境
└── README.md        # 本文件
```

## 快速开始

### 方式一：使用 Docker（推荐）

```bash
# 1. 构建开发镜像
docker build -t os-dev .

# 2. 编译
docker run --rm -v $(pwd):/workspace os-dev make

# 3. 运行（需要本地安装 QEMU）
make run
```

### 方式二：本地编译

```bash
# 安装 nasm（如未安装）
brew install nasm

# 编译
make

# 运行
make run
```

## 运行效果

启动后会看到：
```
Hello, OS!
```

## 代码说明

| 部分 | 说明 |
|------|------|
| `[BITS 16]` | 16位实模式，CPU 启动后的默认状态 |
| `[ORG 0x7C00]` | BIOS 将引导扇区加载到内存 0x7C00 |
| `int 0x10` | BIOS 视频中断，用于输出字符 |
| `0xAA55` | 引导签名，BIOS 以此识别可引导设备 |

## 快捷键

- QEMU 中按 `Ctrl+A` 然后 `X` 退出
- 或 `Ctrl+C` 强制退出

## 下一步

1. 添加更多输出功能
2. 切换到保护模式（32位）
3. 加载内核
