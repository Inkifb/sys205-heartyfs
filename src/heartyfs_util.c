#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "heartyfs.h"

int find_directory(void *disk, const char *path, struct heartyfs_directory **dir) {
    struct heartyfs_directory *current_dir = (struct heartyfs_directory *)disk;
    char *token, *path_copy = strdup(path);
    token = strtok(path_copy, "/");
    
    while (token != NULL) {
        int found = 0;
        for (int i = 0; i < current_dir->size; i++) {
            if (strcmp(current_dir->entries[i].file_name, token) == 0) {
                current_dir = (struct heartyfs_directory *)(disk + current_dir->entries[i].block_id * BLOCK_SIZE);
                found = 1;
                break;
            }
        }
        if (!found) {
            free(path_copy);
            return -1;
        }
        token = strtok(NULL, "/");
    }
    
    *dir = current_dir;
    free(path_copy);
    return 0;
}