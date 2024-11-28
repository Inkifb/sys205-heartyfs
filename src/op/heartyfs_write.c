#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "../heartyfs.h"
#include "../heartyfs_bitmap.h"

int get_free_block_count(void *disk) {
    unsigned char *bitmap = (unsigned char *)disk + BLOCK_SIZE; // Bitmap starts at second block
    int free_count = 0;

    // Start from block 2 (index 0 in the loop) as blocks 0 and 1 are always used
    for (int i = 2; i < NUM_BLOCK; i++) {
        int byte_index = i / 8;
        int bit_index = i % 8;
        if (bitmap[byte_index] & (1 << bit_index)) {
            free_count++;
        }
    }

    return free_count;
}

int heartyfs_write(const char *path, const char *source_path) {

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

    // Open the source file
    FILE *source_file = fopen(source_path, "rb");
    if (source_file == NULL) {
        perror("Error opening source file");
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    // Get the size of the source file
    fseek(source_file, 0, SEEK_END);
    long source_size = ftell(source_file);
    fseek(source_file, 0, SEEK_SET);

    long max_content_size =  get_free_block_count(disk) * CONTEXT_SIZE;

    if (source_size > max_content_size) {
        fprintf(stderr, "Source file too large (max size: %li bytes curr: %li)\n", max_content_size, source_size);
        fclose(source_file);
        munmap(disk, NUM_BLOCK * BLOCK_SIZE);
        close(fd);
        return -1;
    }

    // Free existing data blocks
    for (int i = 0; i < 119 && file_inode->data_blocks[i] != 0; i++) {
        mark_block_free(disk, file_inode->data_blocks[i]);
        file_inode->data_blocks[i] = 0;
    }

    // Write new data
    char buffer[BLOCK_SIZE - 4];
    int blocks_needed = (source_size + BLOCK_SIZE - 5) / (BLOCK_SIZE - 4);
    for (int i = 0; i < blocks_needed; i++) {
        int new_block = find_free_block(disk);
        if (new_block == -1) {
            fprintf(stderr, "No free blocks available\n");
            fclose(source_file);
            munmap(disk, NUM_BLOCK * BLOCK_SIZE);
            close(fd);
            return -1;
        }
        mark_block_used(disk, new_block);
        file_inode->data_blocks[i] = new_block;

        struct heartyfs_data_block *data_block = (struct heartyfs_data_block *)(disk + new_block * BLOCK_SIZE);
        size_t bytes_read = fread(buffer, 1, BLOCK_SIZE - 4, source_file);
        memcpy(data_block->content, buffer, bytes_read);
        data_block->size = bytes_read;
    }

    file_inode->size = source_size;

    fclose(source_file);

    if (msync(disk, NUM_BLOCK * BLOCK_SIZE, MS_SYNC) == -1) {
        perror("Error syncing changes to disk");
    }

    munmap(disk, NUM_BLOCK * BLOCK_SIZE);
    close(fd);

    printf("File written successfully: %s (Size: %ld bytes)\n", path, source_size);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <heartyfs_file_path> <source_file_path>\n", argv[0]);
        return 1;
    }

    return heartyfs_write(argv[1], argv[2]);
}