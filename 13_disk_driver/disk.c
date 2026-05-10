/* disk.c - ATA/IDE 磁盘驱动实现 */

#include <stddef.h>
#include "disk.h"

/* 端口 I/O */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* 简单延时 */
static void disk_delay(void) {
    for (volatile int i = 0; i < 10000; i++);
}

/* 等待磁盘就绪 */
static int wait_for_ready(disk_t *disk) {
    uint16_t status_port = disk->ctrl_base + ATA_CTRL_ALT_STATUS;

    /* 等待 BSY 清除 */
    for (int timeout = 100000; timeout > 0; timeout--) {
        uint8_t status = inb(status_port);
        if (!(status & ATA_STATUS_BSY)) {
            return 0;
        }
    }
    return -1;  /* 超时 */
}

/* 等待数据请求 */
static int wait_for_drq(disk_t *disk) {
    uint16_t status_port = disk->io_base + ATA_REG_STATUS;

    for (int timeout = 100000; timeout > 0; timeout--) {
        uint8_t status = inb(status_port);
        if (status & ATA_STATUS_ERR) {
            return -1;  /* 错误 */
        }
        if (status & ATA_STATUS_DRQ) {
            return 0;  /* 数据就绪 */
        }
        if (!(status & ATA_STATUS_BSY) && !(status & ATA_STATUS_DRQ)) {
            return -1;  /* 无数据请求 */
        }
    }
    return -1;  /* 超时 */
}

/* 选择驱动器 */
static void select_drive(disk_t *disk, uint8_t lba_bits) {
    uint8_t drive_sel = ATA_DRIVE_LBA | disk->drive;
    if (lba_bits > 24) {
        /* LBA48 模式 */
    }
    outb(disk->io_base + ATA_REG_DRIVE, drive_sel);
    disk_delay();
}

/* 转换字符串（IDE 返回的字符串是字交换的） */
static void convert_string(char *dest, const uint16_t *src, int words) {
    for (int i = 0; i < words; i++) {
        dest[i * 2] = (char)(src[i] >> 8);
        dest[i * 2 + 1] = (char)(src[i] & 0xFF);
    }
    dest[words * 2] = '\0';

    /* 去除尾部空格 */
    for (int i = words * 2 - 1; i >= 0 && dest[i] == ' '; i--) {
        dest[i] = '\0';
    }
}

/* 检测磁盘 */
int disk_detect(disk_t *disk, uint16_t io_base, uint16_t ctrl_base, uint8_t drive) {
    disk->io_base = io_base;
    disk->ctrl_base = ctrl_base;
    disk->drive = (drive == 0) ? ATA_DRIVE_MASTER : ATA_DRIVE_SLAVE;
    disk->is_present = 0;
    disk->sectors = 0;

    /* 选择驱动器 */
    outb(io_base + ATA_REG_DRIVE, disk->drive | ATA_DRIVE_LBA);
    disk_delay();

    /* 检查状态 - 如果状态寄存器返回 0xFF，设备不存在 */
    uint8_t status = inb(io_base + ATA_REG_STATUS);
    if (status == 0xFF) {
        return -1;  /* 设备不存在 */
    }

    /* 发送 IDENTIFY 命令 */
    outb(io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    disk_delay();

    /* 等待 BSY 清除 */
    for (int timeout = 100000; timeout > 0; timeout--) {
        status = inb(io_base + ATA_REG_STATUS);
        if (status == 0) {
            return -1;  /* 设备不存在 */
        }
        if (!(status & ATA_STATUS_BSY)) {
            break;
        }
    }

    if (status & ATA_STATUS_BSY) {
        return -1;  /* 超时 */
    }

    /* 检查错误 */
    if (status & ATA_STATUS_ERR) {
        return -1;  /* 错误或非 ATA 设备 */
    }

    /* 等待 DRQ (数据请求) */
    for (int timeout = 100000; timeout > 0; timeout--) {
        status = inb(io_base + ATA_REG_STATUS);
        if (status & ATA_STATUS_DRQ) {
            break;
        }
        if (status & ATA_STATUS_ERR) {
            return -1;
        }
    }

    if (!(status & ATA_STATUS_DRQ)) {
        return -1;  /* 没有数据 */
    }

    /* 读取识别数据 */
    disk_identify_t identify;
    uint16_t *data = (uint16_t*)&identify;

    for (int i = 0; i < 256; i++) {
        data[i] = inw(io_base + ATA_REG_DATA);
    }

    /* 解析型号 (字 27-46) */
    convert_string(disk->model, &data[IDENTIFY_MODEL_OFFSET], 20);

    /* 解析序列号 (字 10-19) */
    convert_string(disk->serial, &data[IDENTIFY_SERIAL_OFFSET], 10);

    /* 获取扇区数 - 先检查 48 位 LBA */
    uint64_t sectors48 = ((uint64_t)data[IDENTIFY_SECTORS_48 + 3] << 48) |
                         ((uint64_t)data[IDENTIFY_SECTORS_48 + 2] << 32) |
                         ((uint64_t)data[IDENTIFY_SECTORS_48 + 1] << 16) |
                         (uint64_t)data[IDENTIFY_SECTORS_48];

    if (sectors48 != 0) {
        disk->sectors = sectors48;
    } else {
        /* 28 位 LBA (字 60-61) */
        disk->sectors = ((uint32_t)data[IDENTIFY_SECTORS_28 + 1] << 16) |
                        (uint32_t)data[IDENTIFY_SECTORS_28];
    }

    disk->is_present = 1;
    return 0;
}

/* 读取扇区 (LBA28 PIO 模式) */
int disk_read_sectors(disk_t *disk, uint64_t lba, uint32_t count, void *buffer) {
    if (disk == NULL || !disk->is_present || count == 0 || buffer == NULL) {
        return -1;
    }

    /* 检查 LBA 范围 (28 位模式最大 256MB) */
    if (lba + count > 0x0FFFFFFF) {
        return -1;
    }

    uint16_t *buf = (uint16_t*)buffer;

    /* 限制每次最多读取 256 扇区 */
    while (count > 0) {
        uint8_t sectors_to_read = (count > 256) ? 256 : (uint8_t)count;
        if (sectors_to_read == 256) sectors_to_read = 0;  /* 0 表示 256 */

        /* 等待磁盘就绪 */
        if (wait_for_ready(disk) < 0) {
            return -1;
        }

        /* 选择驱动器并设置 LBA */
        uint8_t drive_sel = ATA_DRIVE_LBA | disk->drive;
        drive_sel |= (uint8_t)((lba >> 24) & 0x0F);

        outb(disk->io_base + ATA_REG_DRIVE, drive_sel);
        disk_delay();

        /* 设置 LBA 和扇区计数 */
        outb(disk->io_base + ATA_REG_SECTOR_COUNT, sectors_to_read);
        outb(disk->io_base + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
        outb(disk->io_base + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
        outb(disk->io_base + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

        /* 发送读取命令 */
        outb(disk->io_base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);

        /* 读取数据 */
        uint8_t sectors_read = (sectors_to_read == 0) ? 256 : sectors_to_read;
        for (int s = 0; s < sectors_read; s++) {
            if (wait_for_drq(disk) < 0) {
                return -1;
            }

            /* 读取一个扇区 (256 个字 = 512 字节) */
            for (int i = 0; i < 256; i++) {
                *buf++ = inw(disk->io_base + ATA_REG_DATA);
            }
        }

        lba += sectors_read;
        count -= sectors_read;
    }

    return 0;
}

/* 写入扇区 (LBA28 PIO 模式) */
int disk_write_sectors(disk_t *disk, uint64_t lba, uint32_t count, const void *buffer) {
    if (disk == NULL || !disk->is_present || count == 0 || buffer == NULL) {
        return -1;
    }

    /* 检查 LBA 范围 */
    if (lba + count > 0x0FFFFFFF) {
        return -1;
    }

    const uint16_t *buf = (const uint16_t*)buffer;

    while (count > 0) {
        uint8_t sectors_to_write = (count > 256) ? 256 : (uint8_t)count;
        if (sectors_to_write == 256) sectors_to_write = 0;

        if (wait_for_ready(disk) < 0) {
            return -1;
        }

        /* 选择驱动器并设置 LBA */
        uint8_t drive_sel = ATA_DRIVE_LBA | disk->drive;
        drive_sel |= (uint8_t)((lba >> 24) & 0x0F);

        outb(disk->io_base + ATA_REG_DRIVE, drive_sel);
        disk_delay();

        /* 设置 LBA 和扇区计数 */
        outb(disk->io_base + ATA_REG_SECTOR_COUNT, sectors_to_write);
        outb(disk->io_base + ATA_REG_LBA_LOW, (uint8_t)(lba & 0xFF));
        outb(disk->io_base + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
        outb(disk->io_base + ATA_REG_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

        /* 发送写入命令 */
        outb(disk->io_base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);

        uint8_t sectors_written = (sectors_to_write == 0) ? 256 : sectors_to_write;
        for (int s = 0; s < sectors_written; s++) {
            if (wait_for_drq(disk) < 0) {
                return -1;
            }

            /* 写入一个扇区 */
            for (int i = 0; i < 256; i++) {
                outw(disk->io_base + ATA_REG_DATA, *buf++);
            }
        }

        /* 刷新缓存 */
        outb(disk->io_base + ATA_REG_COMMAND, ATA_CMD_FLUSH_CACHE);
        wait_for_ready(disk);

        lba += sectors_written;
        count -= sectors_written;
    }

    return 0;
}

/* 检测到的磁盘列表 */
static disk_t detected_disks[4];
static int disk_count = 0;

/* 初始化磁盘驱动 */
void disk_init(void) {
    disk_count = 0;

    /* 检测 Primary Master */
    if (disk_detect(&detected_disks[disk_count], ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, 0) == 0) {
        disk_count++;
    }

    /* 检测 Primary Slave */
    if (disk_detect(&detected_disks[disk_count], ATA_PRIMARY_IO, ATA_PRIMARY_CTRL, 1) == 0) {
        disk_count++;
    }

    /* 检测 Secondary Master */
    if (disk_detect(&detected_disks[disk_count], ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, 0) == 0) {
        disk_count++;
    }

    /* 检测 Secondary Slave */
    if (disk_detect(&detected_disks[disk_count], ATA_SECONDARY_IO, ATA_SECONDARY_CTRL, 1) == 0) {
        disk_count++;
    }
}

/* 获取主磁盘 */
disk_t* disk_get_primary(void) {
    if (disk_count > 0) {
        return &detected_disks[0];
    }
    return NULL;
}

/* 获取磁盘数量 */
int disk_get_count(void) {
    return disk_count;
}

/* 打印磁盘信息 */
void disk_print_info(disk_t *disk) {
    if (disk == NULL || !disk->is_present) {
        return;
    }

    /* 这里的打印需要配合 kernel.c 中的 print_string 实现 */
    /* 简化版：只返回信息 */
}
