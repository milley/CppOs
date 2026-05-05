; bootloader.asm - 引导程序（含内存检测）

[BITS 16]
[ORG 0x7C00]

KERNEL_START   equ 0x7E00
KERNEL_SECTORS equ 30
MEMORY_MAP_ADDR equ 0x5000    ; 内存映射存放地址

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    ; 打印消息
    mov si, msg_mem
    call print16

    ; ========== 内存检测 (INT 0x15, EAX=0xE820) ==========
    call detect_memory

    ; 加载内核
    mov si, msg_load
    call print16

    mov ah, 0x02
    mov al, KERNEL_SECTORS
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, 0x00
    mov bx, KERNEL_START
    int 0x13
    jc disk_error

    ; 切换到保护模式
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

    jmp 0x08:pm_entry

; ============== 内存检测函数 ==============
detect_memory:
    pusha                      ; 保存所有寄存器

    ; 初始化条目计数为 0
    mov dword [MEMORY_MAP_ADDR], 0

    ; 设置目标地址
    mov di, MEMORY_MAP_ADDR + 4    ; +4 跳过条目计数
    xor ebx, ebx                   ; 必须从 0 开始

    ; 设置签名 'SMAP'
    mov edx, 0x534D4150

    ; 循环获取内存映射
.mem_loop:
    mov eax, 0xE820         ; 功能号
    mov ecx, 24             ; 缓冲区大小 (24 字节)
    int 0x15

    ; 检查错误
    jc .mem_done            ; CF=1 表示结束
    cmp eax, 0x534D4150     ; 检查签名
    jne .mem_done

    ; 增加条目计数
    inc dword [MEMORY_MAP_ADDR]

    ; 移动到下一个条目位置
    add di, 24

    ; 检查是否结束
    test ebx, ebx
    jnz .mem_loop

.mem_done:
    popa                       ; 恢复所有寄存器
    ret

; ============== 16位打印函数 ==============
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

; ============== 数据 ==============
msg_mem:  db "Detecting memory...", 13, 10, 0
msg_load: db "Loading kernel...", 13, 10, 0
msg_pm:   db "Switching to PM...", 13, 10, 0
msg_err:  db "Disk error!", 13, 10, 0

; ============== GDT ==============
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
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x08:KERNEL_START

.halt:
    hlt
    jmp .halt

times 510 - ($ - $$) db 0
dw 0xAA55
