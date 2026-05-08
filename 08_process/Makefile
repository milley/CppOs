# Makefile - 进程管理

AS = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -c -I. -mno-sse -mno-sse2

BOOTLOADER = bootloader.bin
KERNEL_BIN = kernel.bin
OS_BIN = os.bin

all: $(OS_BIN)

$(BOOTLOADER): bootloader.asm
	$(AS) -f bin bootloader.asm -o bootloader.bin

kernel_entry.o: kernel_entry.asm
	$(AS) -f elf32 kernel_entry.asm -o kernel_entry.o

context.o: context.asm
	$(AS) -f elf32 context.asm -o context.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

process.o: process.c
	$(CC) $(CFLAGS) process.c -o process.o

allocator.o: allocator.c
	$(CC) $(CFLAGS) allocator.c -o allocator.o

$(KERNEL_BIN): kernel_entry.o kernel.o process.o context.o allocator.o
	$(LD) -m elf_i386 -Ttext 0x7E00 -o kernel_elf.elf $^
	$(OBJCOPY) -O binary kernel_elf.elf kernel.bin

$(OS_BIN): $(BOOTLOADER) $(KERNEL_BIN)
	cat bootloader.bin kernel.bin > os.bin
	truncate -s 15360 os.bin

run: $(OS_BIN)
	qemu-system-x86_64 -fda os.bin

clean:
	rm -f *.o *.bin *.elf