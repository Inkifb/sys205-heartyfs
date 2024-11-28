#ifndef HEARTYFS_BITMAP_H
#define HEARTYFS_BITMAP_H

void mark_block_used(void *disk, int block_num);
void mark_block_free(void *disk, int block_num);
int find_free_block(void *disk);
int is_block_free(void *disk, int block_num);

#endif