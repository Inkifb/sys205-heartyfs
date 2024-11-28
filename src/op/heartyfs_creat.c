#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "../heartyfs.h"
#include "../heartyfs_bitmap.h"

int heartyfs_creat(const char *path) {
    int fd = open("/tmp/heartyfs", O_RDWR);
    if (fd == -1) {
        perror("Error opening heartyfs");
        return -1;
    }

    void *disk = mmap(NULL, NUM_BLOCK * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Error mapping heartyfs");
        close(fd);
        return -1;
    }

    char parent_path[MAX_PATH], file_name[29];
    strncpy(parent_path, path, sizeof(parent_path) - 1);
    parent_path[sizeof(parent_path) - 1] = '\0';  // Ensure null-termination

    // Removing the trailing slash
    size_t parent_len = strlen(parent_path);

    char *last_slash = strrchr(parent_path, '/');
    strncpy(file_name, last_slash + 1, sizeof(file_name) - 1);

    while (parent_len > 1 && parent_path[parent_len - 1] != '/')
        parent_path[--parent_len] = '\0';

    printf("File path HERE:%s\n", parent_path);
    printf("File Name HERE:%s\n", file_name);
    //-----------------------------------------------------------------------------------------------------------

    struct heartyfs_directory *parent_dir;
    if (find_directory(disk, parent_path, &parent_dir) != 0) {
        fprintf(stderr, "Parent directory not found: %s\n", parent_path);
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    // Check if file already exists
    for (int i = 0; i < parent_dir->size; i++) {
        if (strcmp(parent_dir->entries[i].file_name, file_name) == 0) {
            fprintf(stderr, "File already exists: %s\n", file_name);
            munmap(disk, NUM_BLOCK * BLOCK_SIZE);
            close(fd);
            return -1;
        }
    }

    // Check if parent directory is full
    if (parent_dir->size >= 14) {
        fprintf(stderr, "Parent directory is full\n");
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    // Find a free block for the new inode
    int inode_block = find_free_block(disk);
    if (inode_block == -1) {
        fprintf(stderr, "No free blocks available\n");
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    // Mark the block as used
    mark_block_used(disk, inode_block);

    // Initialize new inode
    struct heartyfs_inode *new_inode = (struct heartyfs_inode *)(disk + inode_block * BLOCK_SIZE);
    new_inode->type = 0;  // Regular file type
    strncpy(new_inode->name, file_name, sizeof(new_inode->name) - 1);
    new_inode->size = 0;  // Initially empty file
    memset(new_inode->data_blocks, 0, sizeof(new_inode->data_blocks));

    // Add new entry to parent directory
    parent_dir->entries[parent_dir->size].block_id = inode_block;
    strncpy(parent_dir->entries[parent_dir->size].file_name, file_name, sizeof(parent_dir->entries[parent_dir->size].file_name) - 1);
    parent_dir->size++;

    if (msync(disk, NUM_BLOCK * BLOCK_SIZE, MS_SYNC) == -1) {
        perror("Error syncing changes to disk");
    }

    munmap(disk, NUM_BLOCK * BLOCK_SIZE);
    close(fd);

    printf("File created successfully: %s\n", path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file_path>\n", argv[0]);
        return 1;
    }

    return heartyfs_creat(argv[1]);
}