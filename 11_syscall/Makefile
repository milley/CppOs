# Makefile - 系统调用

AS = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -c -I. -mno-sse -mno-sse2

all: os.bin

bootloader.bin: bootloader.asm
	$(AS) -f bin bootloader.asm -o bootloader.bin

kernel_entry.o: kernel_entry.asm
	$(AS) -f elf32 kernel_entry.asm -o kernel_entry.o

idt_asm.o: idt_asm.asm
	$(AS) -f elf32 idt_asm.asm -o idt_asm.o

usermode_asm.o: usermode.asm
	$(AS) -f elf32 usermode.asm -o usermode_asm.o

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

kernel.bin: kernel_entry.o kernel.o idt.o idt_asm.o pic.o syscall.o usermode.o usermode_asm.o tss.o allocator.o
	$(LD) -m elf_i386 -Ttext 0x7E00 -o kernel_elf.elf $^
	$(OBJCOPY) -O binary kernel_elf.elf kernel.bin

os.bin: bootloader.bin kernel.bin
	cat bootloader.bin kernel.bin > os.bin
	truncate -s 15360 os.bin

run: os.bin
	qemu-system-x86_64 -drive format=raw,file=os.bin,if=floppy

clean:
	rm -f *.o *.bin *.elf