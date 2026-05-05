# Makefile - 内存检测内核

ASM = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
QEMU = qemu-system-x86_64

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -c -I. -mno-sse -mno-sse2

BOOT_BIN = bootloader.bin
KERNEL_BIN = kernel.bin
OS_BIN = os.bin

all: $(OS_BIN)

$(BOOT_BIN): bootloader.asm
	$(ASM) -f bin bootloader.asm -o $(BOOT_BIN)

kernel_entry.o: kernel_entry.asm
	$(ASM) -f elf32 kernel_entry.asm -o kernel_entry.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

memory.o: memory.c
	$(CC) $(CFLAGS) memory.c -o memory.o

$(KERNEL_BIN): kernel_entry.o kernel.o memory.o
	$(LD) -m elf_i386 -Ttext 0x7E00 -o kernel_elf.elf $^
	$(OBJCOPY) -O binary kernel_elf.elf $(KERNEL_BIN)

$(OS_BIN): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_BIN)
	truncate -s 15360 $(OS_BIN)

run: $(OS_BIN)
	$(QEMU) -fda $(OS_BIN)

run-cli: $(OS_BIN)
	$(QEMU) -fda $(OS_BIN) -nographic

clean:
	rm -f *.o *.bin *.elf

.PHONY: all run run-cli clean
