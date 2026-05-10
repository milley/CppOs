# Makefile - 磁盘驱动

AS = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -c -I. -mno-sse -mno-sse2

all: os.bin

# 简单测试
simple: simple_kernel.bin
	nasm -f bin bootloader.asm -o bootloader.bin
	cat bootloader.bin simple_kernel.bin > os.bin
	truncate -s 2560 os.bin

simple_kernel.bin: simple_kernel.c simple_entry.asm
	nasm -f elf32 simple_entry.asm -o simple_entry.o
	$(CC) $(CFLAGS) simple_kernel.c -o simple_kernel.o
	$(LD) -m elf_i386 -Ttext 0x7E00 -o simple_elf.elf simple_entry.o simple_kernel.o
	$(OBJCOPY) -O binary simple_elf.elf simple_kernel.bin

bootloader.bin: bootloader.asm
	$(AS) -f bin bootloader.asm -o bootloader.bin

kernel_entry.o: kernel_entry.asm
	$(AS) -f elf32 kernel_entry.asm -o kernel_entry.o

idt_asm.o: idt_asm.asm
	$(AS) -f elf32 idt_asm.asm -o idt_asm.o

usermode_asm.o: usermode.asm
	$(AS) -f elf32 usermode.asm -o usermode_asm.o

context_switch.o: context_switch.asm
	$(AS) -f elf32 context_switch.asm -o context_switch.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

idt.o: idt.c
	$(CC) $(CFLAGS) idt.c -o idt.o

pic.o: pic.c
	$(CC) $(CFLAGS) pic.c -o pic.o

syscall.o: syscall.c
	$(CC) $(CFLAGS) syscall.c -o syscall.o

usermode.o: usermode.c
	$(CC) $(CFLAGS) usermode.c -o usermode.o

tss.o: tss.c
	$(CC) $(CFLAGS) tss.c -o tss.o

allocator.o: allocator.c
	$(CC) $(CFLAGS) allocator.c -o allocator.o

paging.o: paging.c paging.h
	$(CC) $(CFLAGS) paging.c -o paging.o

process.o: process.c process.h paging.h
	$(CC) $(CFLAGS) process.c -o process.o

disk.o: disk.c disk.h
	$(CC) $(CFLAGS) disk.c -o disk.o

string.o: string.c string.h
	$(CC) $(CFLAGS) string.c -o string.o

filesystem.o: filesystem.c filesystem.h
	$(CC) $(CFLAGS) filesystem.c -o filesystem.o

keyboard.o: keyboard.c keyboard.h
	$(CC) $(CFLAGS) keyboard.c -o keyboard.o

shell.o: shell.c shell.h
	$(CC) $(CFLAGS) shell.c -o shell.o

kernel.bin: kernel_entry.o kernel.o idt.o idt_asm.o pic.o syscall.o \
            usermode.o usermode_asm.o tss.o allocator.o paging.o \
            process.o context_switch.o disk.o string.o filesystem.o \
            keyboard.o shell.o
	$(LD) -m elf_i386 -Ttext 0x7E00 -o kernel_elf.elf $^
	$(OBJCOPY) -O binary kernel_elf.elf kernel.bin

os.bin: bootloader.bin kernel.bin
	cat bootloader.bin kernel.bin > os.bin
	truncate -s 29184 os.bin

run: os.bin disk.img
	qemu-system-x86_64 -drive format=raw,file=os.bin,if=floppy -drive format=raw,file=disk.img,if=ide -boot a -nographic -serial mon:stdio

run-graphic: os.bin disk.img
	qemu-system-x86_64 -drive format=raw,file=os.bin,if=floppy -drive format=raw,file=disk.img,if=ide -boot a

disk.img:
	dd if=/dev/zero of=disk.img bs=1M count=32 2>/dev/null

clean:
	rm -f *.o *.bin *.elf
