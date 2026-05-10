/* disk.h - ATA/IDE 磁盘驱动头文件 */

#ifndef DISK_H
#define DISK_H

#include <stdint.h>

/* ATA 寄存器端口 (Primary Channel) */
#define ATA_PRIMARY_IO       0x1F0
#define ATA_PRIMARY_CTRL     0x3F6

/* ATA 寄存器端口 (Secondary Channel) */
#define ATA_SECONDARY_IO     0x170
#define ATA_SECONDARY_CTRL   0x376

/* ATA 寄存器偏移 */
#define ATA_REG_DATA         0   /* 数据寄存器 */
#define ATA_REG_ERROR        1   /* 错误寄存器 */
#define ATA_REG_FEATURES     1   /* 特性寄存器 */
#define ATA_REG_SECTOR_COUNT 2   /* 扇区计数 */
#define ATA_REG_LBA_LOW      3   /* LBA 低 8 位 */
#define ATA_REG_LBA_MID      4   /* LBA 中 8 位 */
#define ATA_REG_LBA_HIGH     5   /* LBA 高 8 位 */
#define ATA_REG_DRIVE        6   /* 驱动器/磁头 */
#define ATA_REG_STATUS       7   /* 状态寄存器 */
#define ATA_REG_COMMAND      7   /* 命令寄存器 */

/* ATA 控制寄存器偏移 */
#define ATA_CTRL_ALT_STATUS  0   /* 替代状态 */
#define ATA_CTRL_DEVICE_CTRL 0   /* 设备控制 */

/* ATA 状态位 */
#define ATA_STATUS_ERR       0x01  /* 错误 */
#define ATA_STATUS_DRQ       0x08  /* 数据请求就绪 */
#define ATA_STATUS_SRV       0x10  /* 服务请求 */
#define ATA_STATUS_DF        0x20  /* 驱动器故障 */
#define ATA_STATUS_RDY       0x40  /* 就绪 */
#define ATA_STATUS_BSY       0x80  /* 忙 */

/* ATA 命令 */
#define ATA_CMD_READ_PIO     0x20  /* PIO 读取 */
#define ATA_CMD_READ_PIO_EXT 0x24  /* LBA48 PIO 读取 */
#define ATA_CMD_WRITE_PIO    0x30  /* PIO 写入 */
#define ATA_CMD_WRITE_PIO_EXT 0x34 /* LBA48 PIO 写入 */
#define ATA_CMD_IDENTIFY     0xEC  /* 识别设备 */
#define ATA_CMD_FLUSH_CACHE  0xE7  /* 刷新缓存 */

/* 驱动器选择位 */
#define ATA_DRIVE_MASTER     0x00  /* 主驱动器 */
#define ATA_DRIVE_SLAVE      0x10  /* 从驱动器 */
#define ATA_DRIVE_LBA        0x40  /* LBA 模式 */
#define ATA_DRIVE_LBA48      0x40  /* LBA48 模式 */

/* 扇区大小 */
#define DISK_SECTOR_SIZE     512

/* 磁盘信息结构 (ATA IDENTIFY DEVICE data) */
typedef struct {
    uint16_t words[256];      /* 整个 512 字节 */
} __attribute__((packed)) disk_identify_t;

/* 重要的字偏移量 */
#define IDENTIFY_MODEL_OFFSET      27   /* 型号: 字 27-46 */
#define IDENTIFY_SERIAL_OFFSET     10   /* 序列号: 字 10-19 */
#define IDENTIFY_SECTORS_28        60   /* 28位 LBA 扇区数: 字 60-61 */
#define IDENTIFY_SECTORS_48        100  /* 48位 LBA 扇区数: 字 100-103 */

/* 磁盘设备结构 */
typedef struct {
    uint16_t io_base;        /* I/O 基地址 */
    uint16_t ctrl_base;      /* 控制端口基地址 */
    uint8_t  drive;          /* 主/从驱动器 */
    uint8_t  is_present;     /* 设备是否存在 */
    uint64_t sectors;        /* 总扇区数 */
    char     model[41];      /* 型号字符串 */
    char     serial[21];     /* 序列号 */
} disk_t;

/* 函数声明 */

/* 初始化磁盘驱动 */
void disk_init(void);

/* 检测磁盘 */
int disk_detect(disk_t *disk, uint16_t io_base, uint16_t ctrl_base, uint8_t drive);

/* 读取扇区 */
int disk_read_sectors(disk_t *disk, uint64_t lba, uint32_t count, void *buffer);

/* 写入扇区 */
int disk_write_sectors(disk_t *disk, uint64_t lba, uint32_t count, const void *buffer);

/* 获取主磁盘 */
disk_t* disk_get_primary(void);

/* 获取磁盘数量 */
int disk_get_count(void);

/* 打印磁盘信息 */
void disk_print_info(disk_t *disk);

#endif
