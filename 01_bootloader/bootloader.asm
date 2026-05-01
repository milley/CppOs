; bootloader.asm - 最简单的引导程序
; 功能：在屏幕上显示 "Hello, OS!"

[BITS 16]           ; 16位实模式
[ORG 0x7C00]        ; BIOS加载引导扇区到 0x7C00

start:
    ; 设置段寄存器
    xor ax, ax
    mov ds, ax
    mov es, ax

    ; 打印消息
    mov si, message
print_loop:
    lodsb           ; 加载 SI 指向的字符到 AL
    or al, al       ; 检查是否为 0（字符串结束）
    jz halt
    mov ah, 0x0E    ; BIOS teletype 模式
    int 0x10        ; BIOS 视频中断
    jmp print_loop

halt:
    hlt             ; 停机
    jmp halt        ; 防止被中断唤醒后继续执行

message:
    db "Hello, OS!", 13, 10, 0   ; CR, LF, NULL

; 填充到 510 字节，并添加引导签名
times 510 - ($ - $$) db 0
dw 0xAA55           ; 引导签名（小端序：55 AA）
