; bootloader.asm - 加载内核并切换到保护模式

[BITS 16]
[ORG 0x7C00]

KERNEL_START equ 0x7E00      ; 内核加载地址
KERNEL_SECTORS equ 10        ; 内核占用的扇区数

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; 打印加载消息
    mov si, msg_load
    call print16

    ; 从磁盘加载内核（扇区 2 开始）
    mov ah, 0x02            ; BIOS 读扇区
    mov al, KERNEL_SECTORS  ; 读取扇区数
    mov ch, 0               ; 柱面 0
    mov cl, 2               ; 从扇区 2 开始
    mov dh, 0               ; 磁头 0
    mov dl, 0x00            ; 软盘 A:
    mov bx, KERNEL_START    ; 目标地址
    int 0x13
    jc disk_error           ; 读取失败则跳转

    ; 打印切换消息
    mov si, msg_pm
    call print16

    ; 启用 A20
    in al, 0x92
    or al, 2
    out 0x92, al

    ; 加载 GDT
    lgdt [gdt_ptr]

    ; 进入保护模式
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; 远跳转到保护模式
    jmp 0x08:pm_entry

disk_error:
    mov si, msg_err
    call print16
.halt:
    hlt
    jmp .halt

print16:
    lodsb
    test al, al
    jz .ret
    mov ah, 0x0E
    int 0x10
    jmp print16
.ret:
    ret

msg_load: db "Loading kernel...", 13, 10, 0
msg_pm: db "Switching to PM...", 13, 10, 0
msg_err: db "Disk error!", 13, 10, 0

; GDT
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

; ============== 32位保护模式 ==============
[BITS 32]
pm_entry:
    ; 设置数据段
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000

    ; 跳转到内核
    jmp KERNEL_START

.halt:
    hlt
    jmp .halt

times 510 - ($ - $$) db 0
dw 0xAA55
