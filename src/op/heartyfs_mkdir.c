#include "../heartyfs.h"
#include "../heartyfs_bitmap.h"
#include <string.h>
#include <unistd.h>

int heartyfs_mkdir(const char *path) {
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

    // Find parent directory
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
        fprintf(stderr, "Parent directory not found\n");
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    // Check if directory already exists
    for (int i = 0; i < parent_dir->size; i++) {
        if (strcmp(parent_dir->entries[i].file_name, dir_name) == 0) {
            
            fprintf(stderr, "Directory already exists\n");
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

    // Find a free block for the new directory
    int new_dir_block = find_free_block(disk);
    if (new_dir_block == -1) {
        fprintf(stderr, "No free blocks available\n");
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    // Mark the block as used
    mark_block_used(disk, new_dir_block);

    // Initialize new directory
    struct heartyfs_directory *new_dir = (struct heartyfs_directory *)(disk + new_dir_block * BLOCK_SIZE);
    new_dir->type = 1; // Directory type
    strncpy(new_dir->name, dir_name, sizeof(new_dir->name) - 1);
    new_dir->size = 2; // . and ..

    // Set up . entry
    new_dir->entries[0].block_id = new_dir_block;
    strcpy(new_dir->entries[0].file_name, ".");

    // Set up .. entry
    new_dir->entries[1].block_id = ((void *)parent_dir - disk) / BLOCK_SIZE;
    strcpy(new_dir->entries[1].file_name, "..");

    // Add new entry to parent directory
    parent_dir->entries[parent_dir->size].block_id = new_dir_block;
    strncpy(parent_dir->entries[parent_dir->size].file_name, dir_name, sizeof(parent_dir->entries[parent_dir->size].file_name) - 1);
    parent_dir->size++;

    // Sync changes to disk
    if (msync(disk, NUM_BLOCK * BLOCK_SIZE, MS_SYNC) == -1) {
        perror("Error syncing changes to disk");
    }

    // Clean up
    munmap(disk, NUM_BLOCK * BLOCK_SIZE);
    close(fd);

    printf("Directory %s created successfully\n", path);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <directory_path>\n", argv[0]);
        return 1;
    }

    return heartyfs_mkdir(argv[1]);
}
