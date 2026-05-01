# Makefile - 键盘驱动内核

ASM = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
OBJCOPY = x86_64-elf-objcopy
QEMU = qemu-system-x86_64

CFLAGS = -m32 -ffreestanding -fno-pie -nostdlib -c -I.

BOOT_BIN = bootloader.bin
KERNEL_BIN = kernel.bin
OS_BIN = os.bin

all: $(OS_BIN)

$(BOOT_BIN): bootloader.asm
	$(ASM) -f bin bootloader.asm -o $(BOOT_BIN)

idt_asm.o: idt.asm
	$(ASM) -f elf32 idt.asm -o idt_asm.o

isr_asm.o: isr.asm
	$(ASM) -f elf32 isr.asm -o isr_asm.o

kernel_entry.o: kernel_entry.asm
	$(ASM) -f elf32 kernel_entry.asm -o kernel_entry.o

idt_load.o: idt_load.asm
	$(ASM) -f elf32 idt_load.asm -o idt_load.o

kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

idt.o: idt.c
	$(CC) $(CFLAGS) idt.c -o idt.o

isr.o: isr.c
	$(CC) $(CFLAGS) isr.c -o isr.o

pic.o: pic.c
	$(CC) $(CFLAGS) pic.c -o pic.o

keyboard.o: keyboard.c
	$(CC) $(CFLAGS) keyboard.c -o keyboard.o

$(KERNEL_BIN): kernel_entry.o idt_asm.o isr_asm.o kernel.o idt.o isr.o pic.o keyboard.o idt_load.o
	$(LD) -m elf_i386 -Ttext 0x7E00 -o kernel_elf.elf $^
	$(OBJCOPY) -O binary kernel_elf.elf $(KERNEL_BIN)

$(OS_BIN): $(BOOT_BIN) $(KERNEL_BIN)
	cat $(BOOT_BIN) $(KERNEL_BIN) > $(OS_BIN)
	truncate -s 12288 $(OS_BIN)

run: $(OS_BIN)
	$(QEMU) -fda $(OS_BIN)

run-cli: $(OS_BIN)
	$(QEMU) -fda $(OS_BIN) -nographic

clean:
	rm -f *.o *.bin *.elf

.PHONY: all run run-cli clean
