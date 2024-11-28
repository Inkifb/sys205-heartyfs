#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define DISK_FILE_PATH "/tmp/heartyfs"
#define BLOCK_SIZE (512)
#define DISK_SIZE (1048576)
#define NUM_BLOCK (DISK_SIZE / BLOCK_SIZE)
#define BITMAP_BYTES (NUM_BLOCK / 8)
#define MAX_PATH 256
#define CONTEXT_SIZE 508

struct heartyfs_dir_entry {
    int block_id;           // 4 bytes
    char file_name[28];     // 28 bytes
};  // Overall: 32 bytes

struct heartyfs_directory {
    int type;               // 4 bytes
    char name[28];          // 28 bytes
    int size;               // 4 bytes
    struct heartyfs_dir_entry entries[14]; //14 bytes
};  // Overall: 50 bytes

struct heartyfs_inode {
    int type;               // 4 bytes
    char name[28];          // 28 bytes
    int size;               // 4 bytes
    int data_blocks[119];   // 476 bytes
};  // Overall: 512 bytes

struct heartyfs_data_block {
    int size;               // 4 bytes
    char content[508];         // 508 bytes
};  // Overall: 512 bytes


int find_directory(void *disk, const char *path, struct heartyfs_directory **dir);