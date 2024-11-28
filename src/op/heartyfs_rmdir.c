#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "../heartyfs.h"
#include "../heartyfs_bitmap.h"

int heartyfs_rmdir(const char *path) {
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

    char parent_path[MAX_PATH], dir_name[29];
    strncpy(parent_path, path, sizeof(parent_path) - 1);
    parent_path[sizeof(parent_path) - 1] = '\0';  // Ensure null-termination

    // Removing the trailing slash
    size_t parent_len = strlen(parent_path);
    if (parent_len > 1 && parent_path[parent_len - 1] == '/') 
        parent_path[--parent_len] = '\0';

    char *last_slash = strrchr(parent_path, '/');
    strncpy(dir_name, last_slash + 1, sizeof(dir_name) - 1);

    while (parent_len > 1 && parent_path[parent_len - 1] != '/')
        parent_path[--parent_len] = '\0';

    printf("File path HERE:%s\n", parent_path);
    printf("File Name HERE:%s\n", dir_name);
    //-----------------------------------------------------------------------------------------------------------

    dir_name[sizeof(dir_name) - 1] = '\0';  // Ensure null-termination

    struct heartyfs_directory *parent_dir;
    if (find_directory(disk, parent_path, &parent_dir) != 0) {
        fprintf(stderr, "Parent directory not found: %s\n", parent_path);
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    int dir_index = -1, dir_block = -1;
    for (int i = 0; i < parent_dir->size; i++) {
        if (strcmp(parent_dir->entries[i].file_name, dir_name) == 0) {
            dir_index = i;
            dir_block = parent_dir->entries[i].block_id;
            break;
        }
    }

    if (dir_index == -1) {
        fprintf(stderr, "Directory not found: %s\n", dir_name);
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    struct heartyfs_directory *dir_to_remove = (struct heartyfs_directory *)(disk + dir_block * BLOCK_SIZE);
    
    if (dir_to_remove->type != 1) {
        fprintf(stderr, "Not a directory: %s\n", dir_name);
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    if (dir_to_remove->size > 2) {
        fprintf(stderr, "Directory not empty: %s\n", dir_name);
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    // Remove the directory entry from the parent
    for (int i = dir_index; i < parent_dir->size - 1; i++) {
        parent_dir->entries[i] = parent_dir->entries[i + 1];
    }
    parent_dir->size--;

    // Mark the block as free in the bitmap
    mark_block_free(disk, dir_block);

    // Clear the directory block
    memset(dir_to_remove, 0, BLOCK_SIZE);

    if (msync(disk, NUM_BLOCK * BLOCK_SIZE, MS_SYNC) == -1) {
        perror("Error syncing changes to disk");
    }

    munmap(disk, NUM_BLOCK * BLOCK_SIZE);
    close(fd);

    printf("Directory removed successfully: %s\n", path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    return heartyfs_rmdir(argv[1]);
}