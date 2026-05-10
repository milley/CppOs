/* filesystem.c - 简单文件系统实现 */

#include "filesystem.h"
#include "string.h"
#include "allocator.h"

/* 当前磁盘 */
static disk_t *fs_disk = NULL;

/* 文件系统信息 */
static superblock_t superblock;
static uint16_t *fat = NULL;           /* FAT 缓存 */
static dir_entry_t *root_dir = NULL;   /* 根目录缓存 */
static file_descriptor_t fd_table[FS_MAX_OPEN];

/* 扇区缓冲区 */
static uint8_t sector_buffer[FS_CLUSTER_SIZE];

/* ================== 底层函数 ================== */

/* 簇号转扇区号 */
static uint32_t cluster_to_sector(uint32_t cluster) {
    return superblock.data_start + cluster;
}

/* 读取 FAT 项 */
static uint16_t fat_read(uint32_t cluster) {
    if (cluster < superblock.total_clusters) {
        return fat[cluster];
    }
    return FAT_BAD;
}

/* 写入 FAT 项 */
static void fat_write(uint32_t cluster, uint16_t value) {
    if (cluster < superblock.total_clusters) {
        fat[cluster] = value;
    }
}

/* 分配一个空闲簇 */
static int32_t alloc_cluster(void) {
    if (superblock.free_clusters == 0) {
        return -1;  /* 没有空闲簇 */
    }

    /* 从簇 0 开始搜索 */
    for (uint32_t i = 0; i < superblock.total_clusters; i++) {
        if (fat[i] == FAT_FREE) {
            fat[i] = FAT_EOF;
            superblock.free_clusters--;
            return i;
        }
    }
    return -1;
}

/* 释放簇链 */
static void free_cluster_chain(uint32_t start_cluster) {
    uint32_t cluster = start_cluster;
    while (cluster < superblock.total_clusters && fat[cluster] != FAT_FREE) {
        uint16_t next = fat[cluster];
        fat[cluster] = FAT_FREE;
        superblock.free_clusters++;
        if (next >= FAT_EOF) break;
        cluster = next;
    }
}

/* 查找空闲目录项 */
static int find_free_dir_entry(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (root_dir[i].flags == 0) {
            return i;
        }
    }
    return -1;
}

/* 按名称查找文件 */
static int find_file_by_name(const char *name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (root_dir[i].flags == 1 && strcmp(root_dir[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

/* 查找空闲文件描述符 */
static int find_free_fd(void) {
    for (int i = 0; i < FS_MAX_OPEN; i++) {
        if (fd_table[i].in_use == 0) {
            return i;
        }
    }
    return -1;
}

/* ================== 初始化函数 ================== */

void fs_init(disk_t *disk) {
    fs_disk = disk;

    /* 清空文件描述符表 */
    for (int i = 0; i < FS_MAX_OPEN; i++) {
        fd_table[i].in_use = 0;
    }

    /* 读取超级块 */
    if (disk_read_sectors(disk, 0, 1, &superblock) != 0) {
        return;
    }

    /* 检查魔数 */
    if (superblock.magic != FS_MAGIC) {
        return;  /* 未格式化 */
    }

    /* 分配 FAT 缓存 */
    uint32_t fat_size = superblock.fat_sectors * FS_CLUSTER_SIZE;
    fat = (uint16_t *)kmalloc(fat_size);
    if (fat == NULL) return;

    /* 读取 FAT */
    disk_read_sectors(disk, superblock.fat_start, superblock.fat_sectors, fat);

    /* 分配根目录缓存 */
    uint32_t root_size = superblock.root_sectors * FS_CLUSTER_SIZE;
    root_dir = (dir_entry_t *)kmalloc(root_size);
    if (root_dir == NULL) return;

    /* 读取根目录 */
    disk_read_sectors(disk, superblock.root_start, superblock.root_sectors, root_dir);
}

int fs_format(disk_t *disk) {
    if (disk == NULL) return -1;

    fs_disk = disk;

    /* 设置超级块 */
    memset(&superblock, 0, sizeof(superblock));
    superblock.magic = FS_MAGIC;
    superblock.version = 1;
    superblock.total_sectors = disk->sectors;
    superblock.fat_start = 1;
    superblock.fat_sectors = 66;
    superblock.root_start = 67;
    superblock.root_sectors = 16;
    superblock.data_start = 83;
    superblock.total_clusters = disk->sectors - 83;
    superblock.free_clusters = superblock.total_clusters;
    superblock.total_files = 0;

    /* 写入超级块 */
    if (disk_write_sectors(disk, 0, 1, &superblock) != 0) {
        return -1;
    }

    /* 分配并初始化 FAT */
    uint32_t fat_size = superblock.fat_sectors * FS_CLUSTER_SIZE;
    fat = (uint16_t *)kmalloc(fat_size);
    if (fat == NULL) return -1;

    memset(fat, 0, fat_size);
    /* 簇 0 和 1 保留 */
    fat[0] = FAT_EOF;
    fat[1] = FAT_EOF;

    /* 写入 FAT */
    disk_write_sectors(disk, superblock.fat_start, superblock.fat_sectors, fat);

    /* 分配并初始化根目录 */
    uint32_t root_size = superblock.root_sectors * FS_CLUSTER_SIZE;
    root_dir = (dir_entry_t *)kmalloc(root_size);
    if (root_dir == NULL) return -1;

    memset(root_dir, 0, root_size);

    /* 写入根目录 */
    disk_write_sectors(disk, superblock.root_start, superblock.root_sectors, root_dir);

    /* 清空文件描述符表 */
    for (int i = 0; i < FS_MAX_OPEN; i++) {
        fd_table[i].in_use = 0;
    }

    return 0;
}

int fs_is_valid(void) {
    return (fs_disk != NULL && superblock.magic == FS_MAGIC);
}

/* ================== 文件操作 ================== */

int fs_create(const char *name) {
    if (!fs_is_valid() || name == NULL) return -1;

    /* 检查文件名长度 */
    if (strlen(name) >= FS_NAME_MAX) return -1;

    /* 检查文件是否已存在 */
    if (find_file_by_name(name) >= 0) return -1;

    /* 查找空闲目录项 */
    int idx = find_free_dir_entry();
    if (idx < 0) return -1;

    /* 创建目录项 */
    strcpy(root_dir[idx].name, name);
    root_dir[idx].first_cluster = 0;  /* 空文件 */
    root_dir[idx].file_size = 0;
    root_dir[idx].attributes = FS_ATTR_FILE;
    root_dir[idx].flags = 1;

    superblock.total_files++;

    /* 同步到磁盘 */
    disk_write_sectors(fs_disk, superblock.root_start, superblock.root_sectors, root_dir);
    disk_write_sectors(fs_disk, 0, 1, &superblock);

    return 0;
}

int fs_open(const char *name, int mode) {
    if (!fs_is_valid() || name == NULL) return -1;

    /* 查找文件 */
    int idx = find_file_by_name(name);
    if (idx < 0) return -1;

    /* 查找空闲文件描述符 */
    int fd = find_free_fd();
    if (fd < 0) return -1;

    /* 设置文件描述符 */
    strcpy(fd_table[fd].name, root_dir[idx].name);
    fd_table[fd].first_cluster = root_dir[idx].first_cluster;
    fd_table[fd].file_size = root_dir[idx].file_size;
    fd_table[fd].position = (mode == FS_MODE_APPEND) ? root_dir[idx].file_size : 0;
    fd_table[fd].current_cluster = root_dir[idx].first_cluster;
    fd_table[fd].mode = mode;
    fd_table[fd].in_use = 1;
    fd_table[fd].modified = 0;

    return fd;
}

int fs_close(int fd) {
    if (fd < 0 || fd >= FS_MAX_OPEN || fd_table[fd].in_use == 0) return -1;

    /* 如果修改过，更新目录项 */
    if (fd_table[fd].modified) {
        int idx = find_file_by_name(fd_table[fd].name);
        if (idx >= 0) {
            root_dir[idx].first_cluster = fd_table[fd].first_cluster;
            root_dir[idx].file_size = fd_table[fd].file_size;
            disk_write_sectors(fs_disk, superblock.root_start, superblock.root_sectors, root_dir);
        }
    }

    fd_table[fd].in_use = 0;
    return 0;
}

int fs_read(int fd, void *buffer, uint32_t count) {
    if (!fs_is_valid()) return -1;
    if (fd < 0 || fd >= FS_MAX_OPEN || fd_table[fd].in_use == 0) return -1;
    if (fd_table[fd].mode != FS_MODE_READ && fd_table[fd].mode != 0) return -1;

    file_descriptor_t *f = &fd_table[fd];

    /* 空文件 */
    if (f->file_size == 0) return 0;

    /* 限制读取大小 */
    if (f->position + count > f->file_size) {
        count = f->file_size - f->position;
    }
    if (count == 0) return 0;

    uint8_t *buf = (uint8_t *)buffer;
    uint32_t bytes_read = 0;
    uint32_t cluster = f->current_cluster;

    while (bytes_read < count) {
        /* 如果需要，找下一个簇 */
        if (f->position % FS_CLUSTER_SIZE == 0 && f->position > 0) {
            uint16_t next = fat_read(cluster);
            if (next >= FAT_EOF) break;
            cluster = next;
            f->current_cluster = cluster;
        }

        /* 读取簇 */
        uint32_t sector = cluster_to_sector(cluster);
        if (disk_read_sectors(fs_disk, sector, 1, sector_buffer) != 0) {
            break;
        }

        /* 计算本簇内读取量 */
        uint32_t offset = f->position % FS_CLUSTER_SIZE;
        uint32_t chunk = FS_CLUSTER_SIZE - offset;
        if (chunk > count - bytes_read) {
            chunk = count - bytes_read;
        }

        memcpy(buf + bytes_read, sector_buffer + offset, chunk);
        bytes_read += chunk;
        f->position += chunk;
    }

    return bytes_read;
}

int fs_write(int fd, const void *buffer, uint32_t count) {
    if (!fs_is_valid()) return -1;
    if (fd < 0 || fd >= FS_MAX_OPEN || fd_table[fd].in_use == 0) return -1;
    if (fd_table[fd].mode == FS_MODE_READ) return -1;

    file_descriptor_t *f = &fd_table[fd];
    const uint8_t *buf = (const uint8_t *)buffer;
    uint32_t bytes_written = 0;

    /* 检查最大文件大小 */
    if (f->position + count > FS_MAX_FILESIZE) {
        count = FS_MAX_FILESIZE - f->position;
    }
    if (count == 0) return 0;

    while (bytes_written < count) {
        /* 如果需要新簇 */
        if (f->current_cluster == 0 ||
            (f->position % FS_CLUSTER_SIZE == 0 && f->position > 0)) {

            int32_t new_cluster = alloc_cluster();
            if (new_cluster < 0) break;

            if (f->first_cluster == 0) {
                f->first_cluster = new_cluster;
                f->current_cluster = new_cluster;
            } else {
                fat_write(f->current_cluster, new_cluster);
                f->current_cluster = new_cluster;
            }
        }

        /* 读取现有簇内容（部分写入时需要） */
        uint32_t sector = cluster_to_sector(f->current_cluster);
        uint32_t offset = f->position % FS_CLUSTER_SIZE;

        if (offset != 0 || count - bytes_written < FS_CLUSTER_SIZE) {
            /* 部分写入，先读取 */
            disk_read_sectors(fs_disk, sector, 1, sector_buffer);
        } else {
            memset(sector_buffer, 0, FS_CLUSTER_SIZE);
        }

        /* 计算本簇内写入量 */
        uint32_t chunk = FS_CLUSTER_SIZE - offset;
        if (chunk > count - bytes_written) {
            chunk = count - bytes_written;
        }

        memcpy(sector_buffer + offset, buf + bytes_written, chunk);

        /* 写入簇 */
        if (disk_write_sectors(fs_disk, sector, 1, sector_buffer) != 0) {
            break;
        }

        bytes_written += chunk;
        f->position += chunk;
    }

    /* 更新文件大小 */
    if (f->position > f->file_size) {
        f->file_size = f->position;
    }

    f->modified = 1;

    /* 同步 FAT */
    disk_write_sectors(fs_disk, superblock.fat_start, superblock.fat_sectors, fat);
    disk_write_sectors(fs_disk, 0, 1, &superblock);

    return bytes_written;
}

int fs_delete(const char *name) {
    if (!fs_is_valid() || name == NULL) return -1;

    int idx = find_file_by_name(name);
    if (idx < 0) return -1;

    /* 释放簇链 */
    if (root_dir[idx].first_cluster != 0) {
        free_cluster_chain(root_dir[idx].first_cluster);
    }

    /* 清除目录项 */
    root_dir[idx].flags = 0;
    root_dir[idx].name[0] = '\0';
    root_dir[idx].first_cluster = 0;
    root_dir[idx].file_size = 0;

    superblock.total_files--;

    /* 同步到磁盘 */
    disk_write_sectors(fs_disk, superblock.fat_start, superblock.fat_sectors, fat);
    disk_write_sectors(fs_disk, superblock.root_start, superblock.root_sectors, root_dir);
    disk_write_sectors(fs_disk, 0, 1, &superblock);

    return 0;
}

int fs_list(void) {
    if (!fs_is_valid()) return -1;

    int count = 0;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (root_dir[i].flags == 1) {
            count++;
        }
    }
    return count;
}

int32_t fs_size(int fd) {
    if (fd < 0 || fd >= FS_MAX_OPEN || fd_table[fd].in_use == 0) return -1;
    return fd_table[fd].file_size;
}

int fs_sync(void) {
    if (!fs_is_valid()) return -1;

    disk_write_sectors(fs_disk, 0, 1, &superblock);
    disk_write_sectors(fs_disk, superblock.fat_start, superblock.fat_sectors, fat);
    disk_write_sectors(fs_disk, superblock.root_start, superblock.root_sectors, root_dir);

    return 0;
}
