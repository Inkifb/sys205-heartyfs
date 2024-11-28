#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "../heartyfs.h"
#include "../heartyfs_bitmap.h"

int heartyfs_read(const char *path) {
    int fd = open("/tmp/heartyfs", O_RDONLY);
    if (fd == -1) {
        perror("Error opening heartyfs");
        return -1;
    }

    void *disk = mmap(NULL, NUM_BLOCK * BLOCK_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
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

    int file_block = -1;
    for (int i = 0; i < parent_dir->size; i++) {
        if (strcmp(parent_dir->entries[i].file_name, file_name) == 0) {
            file_block = parent_dir->entries[i].block_id;
            break;
        }
    }

    if (file_block == -1) {
        fprintf(stderr, "File not found: %s\n", file_name);
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    struct heartyfs_inode *file_inode = (struct heartyfs_inode *)(disk + file_block * BLOCK_SIZE);
    
    if (file_inode->type != 0) {
        fprintf(stderr, "Not a regular file: %s\n", file_name);
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

     // Read data blocks and print to terminal
    for (int i = 0; i < 119 && file_inode->data_blocks[i] != 0; i++) {
        struct heartyfs_data_block *data_block = (struct heartyfs_data_block *)(disk + file_inode->data_blocks[i] * BLOCK_SIZE);
        fwrite(data_block->content, 1, data_block->size, stdout);
    }

    // Print a newline at the end for better formatting
    printf("\n");

    munmap(disk, NUM_BLOCK * BLOCK_SIZE);
    close(fd);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <heartyfs_file_path>\n", argv[0]);
        return 1;
    }

    return heartyfs_read(argv[1]);
}