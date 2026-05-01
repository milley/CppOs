# Makefile - 保护模式内核构建

ASM = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
QEMU = qemu-system-x86_64

# 文件
BOOT_BIN = bootloader.bin
KERNEL_BIN = kernel.bin
OS_BIN = os.bin

# 编译选项
CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -c

all: $(OS_BIN)

# 编译引导程序（纯二进制）
$(BOOT_BIN): bootloader.asm
	$(ASM) -f bin bootloader.asm -o $(BOOT_BIN)

# 编译内核入口
kernel_entry.o: kernel_entry.asm
	$(ASM) -f elf32 kernel_entry.asm -o kernel_entry.o

# 编译 C 内核
kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

# 链接内核（放在 0x7E00，紧跟引导扇区）
$(KERNEL_BIN): kernel_entry.o kernel.o
	$(LD) -m elf_i386 -Ttext 0x7E00 -o kernel_elf.elf kernel_entry.o kernel.o
	$(OBJCOPY) -O binary kernel_elf.elf $(KERNEL_BIN)

# 合并引导程序和内核（创建软盘映像）
$(OS_BIN): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_BIN)
	truncate -s 6144 $(OS_BIN)

# 运行（图形窗口）
run: $(OS_BIN)
	$(QEMU) -fda $(OS_BIN)

# 运行（命令行）
run-cli: $(OS_BIN)
	$(QEMU) -fda $(OS_BIN) -nographic

# 清理
clean:
	rm -f *.o *.bin *.elf *.log

.PHONY: all run run-cli clean
