#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include "heartyfs.h"

void initialize_superblock(void *disk) {
    struct heartyfs_directory *root = (struct heartyfs_directory *)disk;
    
    root->type = 1; // Directory type
    strncpy(root->name, "/", sizeof(root->name));
    root->size = 2; // . and ..
    
    // Set up . entry
    root->entries[0].block_id = 0;
    strncpy(root->entries[0].file_name, ".", sizeof(root->entries[0].file_name));
    
    // Set up .. entry
    root->entries[1].block_id = 0;
    strncpy(root->entries[1].file_name, "..", sizeof(root->entries[1].file_name));
}

void initialize_bitmap(void *disk) {
    char *bitmap = (char *)disk + BLOCK_SIZE;
    
    // Set all bits to 1 (free)
    memset(bitmap, 0xFF, BITMAP_BYTES);
    

    bitmap[0] = 0xFC; 
}

int main() {
    int fd = open("/tmp/heartyfs", O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    void *disk = mmap(NULL, NUM_BLOCK * BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (disk == MAP_FAILED) {
        perror("Error mapping file");
        close(fd);
        return 1;
    }

    // Initialize superblock (Task #0)
    initialize_superblock(disk);

    // Initialize bitmap (Task #1)
    initialize_bitmap(disk);

    // Sync changes to disk
    if (msync(disk, NUM_BLOCK * BLOCK_SIZE, MS_SYNC) == -1) {
        perror("Error syncing to disk");
    }

    // Unmap and close the file
    munmap(disk, NUM_BLOCK * BLOCK_SIZE);
    close(fd);

    printf("heartyfs initialized successfully.\n");
    return 0;
}