#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "heartyfs.h"
#include "heartyfs_bitmap.h"

void print_bitmap(void *disk) {
    unsigned char *bitmap = (unsigned char *)disk + BLOCK_SIZE;
    printf("Bitmap:\n");
    for (int i = 0; i < NUM_BLOCK / 8; i++) {
        for (int j = 0; j < 8; j++) {
            printf("%c", (bitmap[i] & (1 << j)) ? '1' : '0');
        }
        if ((i + 1) % 8 == 0) printf("\n");
    }
    printf("\n");
}

void print_directory(struct heartyfs_directory *dir, int level) {
    for (int i = 0; i < level; i++) printf("  ");
    printf("%s (size: %d)\n", dir->name, dir->size);
    
    for (int i = 0; i < dir->size; i++) {
        for (int j = 0; j < level + 1; j++) printf("  ");
        printf("- %s (block: %d)\n", dir->entries[i].file_name, dir->entries[i].block_id);
    }
}

void traverse_directory(void *disk, int block_num, int level) {
    struct heartyfs_directory *dir = (struct heartyfs_directory *)(disk + block_num * BLOCK_SIZE);
    print_directory(dir, level);
    
    for (int i = 2; i < dir->size; i++) {  // Skip . and ..
        if (dir->entries[i].block_id != 0) {
            struct heartyfs_directory *subdir = (struct heartyfs_directory *)(disk + dir->entries[i].block_id * BLOCK_SIZE);
            if (subdir->type == 1) {  // If it's a directory
                traverse_directory(disk, dir->entries[i].block_id, level + 1);
            }
        }
    }
}

int main() {
    int fd = open("/tmp/heartyfs", O_RDONLY);
    if (fd == -1) {
        perror("Error opening heartyfs");
        return 1;
    }

    void *disk = mmap(NULL, NUM_BLOCK * BLOCK_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Error mapping heartyfs");
        close(fd);
        return 1;
    }

    printf("HeartyFS Inspector\n\n");

    print_bitmap(disk);

    printf("Directory Structure:\n");
    traverse_directory(disk, 0, 0);  // Start from root (block 0)

    munmap(disk, NUM_BLOCK * BLOCK_SIZE);
    close(fd);

    return 0;
}