; bootloader.asm - 引导程序

[BITS 16]
[ORG 0x7C00]

KERNEL_START   equ 0x7E00
KERNEL_SECTORS equ 66

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov si, msg_load
    call print16

    ; 多磁道读取
    ; 第1次: 扇区 2-18 (17扇区), 磁头0, 柱面0
    mov ah, 0x02
    mov al, 17
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x00
    mov bx, KERNEL_START
    int 0x13
    jc disk_error

    ; 第2次: 扇区 1-18 (18扇区), 磁头1, 柱面0
    mov ah, 0x02
    mov al, 18
    mov ch, 0
    mov cl, 1
    mov dh, 1
    mov dl, 0x00
    mov bx, KERNEL_START + 17*512
    int 0x13
    jc disk_error

    ; 第3次: 扇区 1-18 (18扇区), 磁头0, 柱面1
    mov ah, 0x02
    mov al, 18
    mov ch, 1
    mov cl, 1
    mov dh, 0
    mov dl, 0x00
    mov bx, KERNEL_START + 35*512
    int 0x13
    jc disk_error

    ; 第4次: 扇区 1-12 (12扇区), 磁头1, 柱面1
    mov ah, 0x02
    mov al, 12
    mov ch, 1
    mov cl, 1
    mov dh, 1
    mov dl, 0x00
    mov bx, KERNEL_START + 53*512
    int 0x13
    jc disk_error

    mov si, msg_pm
    call print16

    in al, 0x92
    or al, 2
    out 0x92, al

    lgdt [gdt_ptr]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 跳转到 32 位代码设置段寄存器 (pm_entry 在 0x7CE8)
    jmp 0x08:0x7CE8

print16:
    lodsb
    test al, al
    jz .ret
    mov ah, 0x0E
    int 0x10
    jmp print16
.ret:
    ret

disk_error:
    mov si, msg_err
    call print16
.halt:
    hlt
    jmp .halt

msg_load: db "Loading kernel...", 13, 10, 0
msg_pm:   db "Switching to PM...", 13, 10, 0
msg_err:  db "Disk error!", 13, 10, 0

gdt:
    dq 0
.code:
    dw 0xFFFF, 0
    db 0, 0x9A, 0xCF, 0
.data:
    dw 0xFFFF, 0
    db 0, 0x92, 0xCF, 0
.end:

gdt_ptr:
    dw gdt.end - gdt - 1
    dd gdt

[BITS 32]
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x08:0x7E00

times 510 - ($ - $$) db 0
dw 0xAA55
