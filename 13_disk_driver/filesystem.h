/* filesystem.h - 简单文件系统 */

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include "disk.h"

/* 文件系统魔数 */
#define FS_MAGIC        0x53465331  /* "SFS1" */

/* 常量定义 */
#define FS_NAME_MAX     24          /* 最大文件名长度 */
#define FS_MAX_FILES    256         /* 最大文件数 */
#define FS_MAX_OPEN     8           /* 最大同时打开文件数 */
#define FS_CLUSTER_SIZE 512         /* 簇大小 = 1 扇区 */
#define FS_MAX_FILESIZE 65536       /* 最大文件大小 64KB */

/* FAT 特殊值 */
#define FAT_FREE        0x0000      /* 空闲簇 */
#define FAT_EOF         0xFFF8      /* 文件结束 */
#define FAT_BAD         0xFFF7      /* 坏簇 */

/* 打开模式 */
#define FS_MODE_READ    0
#define FS_MODE_WRITE   1
#define FS_MODE_APPEND  2

/* 文件属性 */
#define FS_ATTR_FILE    0x0000
#define FS_ATTR_DIR     0x0001
#define FS_ATTR_HIDDEN  0x0002
#define FS_ATTR_SYSTEM  0x0004

/* 超级块结构 (512 字节) */
typedef struct {
    uint32_t magic;              /* 魔数 */
    uint32_t version;            /* 版本号 */
    uint32_t total_sectors;      /* 总扇区数 */
    uint32_t fat_start;         /* FAT 起始扇区 */
    uint32_t fat_sectors;       /* FAT 占用扇区数 */
    uint32_t root_start;        /* 根目录起始扇区 */
    uint32_t root_sectors;      /* 根目录占用扇区数 */
    uint32_t data_start;        /* 数据区起始扇区 */
    uint32_t total_clusters;    /* 总簇数 */
    uint32_t free_clusters;     /* 空闲簇数 */
    uint32_t total_files;       /* 文件总数 */
    uint8_t  reserved[468];     /* 保留 */
} __attribute__((packed)) superblock_t;

/* 目录项结构 (32 字节) */
typedef struct {
    char     name[24];          /* 文件名 */
    uint32_t first_cluster;    /* 首簇号 */
    uint32_t file_size;        /* 文件大小 */
    uint16_t attributes;       /* 属性 */
    uint16_t flags;            /* 标志: 0=空闲, 1=已用 */
} __attribute__((packed)) dir_entry_t;

/* 文件描述符 */
typedef struct {
    char     name[FS_NAME_MAX];    /* 文件名 */
    uint32_t first_cluster;        /* 首簇 */
    uint32_t file_size;            /* 文件大小 */
    uint32_t position;             /* 当前位置 */
    uint32_t current_cluster;      /* 当前簇 */
    uint8_t  mode;                 /* 打开模式 */
    uint8_t  in_use;               /* 是否使用 */
    uint8_t  modified;             /* 是否修改 */
} file_descriptor_t;

/* 初始化文件系统 */
void fs_init(disk_t *disk);

/* 格式化磁盘 */
int fs_format(disk_t *disk);

/* 检查文件系统是否有效 */
int fs_is_valid(void);

/* 文件操作 */
int fs_create(const char *name);
int fs_open(const char *name, int mode);
int fs_close(int fd);
int fs_read(int fd, void *buffer, uint32_t count);
int fs_write(int fd, const void *buffer, uint32_t count);
int fs_delete(const char *name);
int fs_list(void);
int32_t fs_size(int fd);

/* 同步到磁盘 */
int fs_sync(void);

/* 获取根目录 (供 Shell 使用) */
dir_entry_t* fs_get_root_dir(void);

#endif
