# Makefile - 构建引导程序

ASM = nasm
QEMU = qemu-system-x86_64

# 源文件和输出
SRC = bootloader.asm
BIN = bootloader.bin

# 构建目标
all: $(BIN)

# 汇编引导程序
$(BIN): $(SRC)
	$(ASM) -f bin $(SRC) -o $(BIN)

# 在 QEMU 中运行
run: $(BIN)
	$(QEMU) -drive format=raw,file=$(BIN) -nographic

# 带 GDB 调试支持运行
debug: $(BIN)
	$(QEMU) -drive format=raw,file=$(BIN) -s -S &
	gdb -ex "target remote localhost:1234"

# 清理
clean:
	rm -f $(BIN)

.PHONY: all run debug clean
