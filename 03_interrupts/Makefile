# Makefile - 中断处理内核

ASM = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
QEMU = qemu-system-x86_64

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -c -I.

# 输出文件
BOOT_BIN = bootloader.bin
KERNEL_BIN = kernel.bin
OS_BIN = os.bin

all: $(OS_BIN)

# 编译引导程序
$(BOOT_BIN): bootloader.asm
	$(ASM) -f bin bootloader.asm -o $(BOOT_BIN)

# 编译汇编文件
idt_asm.o: idt.asm
	$(ASM) -f elf32 idt.asm -o idt_asm.o

isr_asm.o: isr.asm
	$(ASM) -f elf32 isr.asm -o isr_asm.o

kernel_entry.o: kernel_entry.asm
	$(ASM) -f elf32 kernel_entry.asm -o kernel_entry.o

idt_load.o: idt_load.asm
	$(ASM) -f elf32 idt_load.asm -o idt_load.o

# 编译 C 文件
kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

idt.o: idt.c
	$(CC) $(CFLAGS) idt.c -o idt.o

isr.o: isr.c
	$(CC) $(CFLAGS) isr.c -o isr.o

pic.o: pic.c
	$(CC) $(CFLAGS) pic.c -o pic.o

timer.o: timer.c
	$(CC) $(CFLAGS) timer.c -o timer.o

# 链接内核
$(KERNEL_BIN): idt_asm.o isr_asm.o kernel.o idt.o isr.o pic.o timer.o kernel_entry.o idt_load.o
	$(LD) -m elf_i386 -Ttext 0x7E00 -o kernel_elf.elf $^
	$(OBJCOPY) -O binary kernel_elf.elf $(KERNEL_BIN)

# 创建磁盘映像
$(OS_BIN): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_BIN)
	truncate -s 12288 $(OS_BIN)

# 运行
run: $(OS_BIN)
	$(QEMU) -fda $(OS_BIN)

# 运行（命令行）
run-cli: $(OS_BIN)
	$(QEMU) -fda $(OS_BIN) -nographic

# 清理
clean:
	rm -f *.o *.bin *.elf

.PHONY: all run run-cli clean