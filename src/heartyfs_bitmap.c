#include "heartyfs.h"
#include "heartyfs_bitmap.h"
#include <stdint.h>
#include <string.h>
#include <unistd.h>

void mark_block_used(void *disk, int block_num) {
    uint8_t *bitmap = (uint8_t *)disk + BLOCK_SIZE;
    int byte_index = block_num / 8;
    int bit_index = block_num % 8;
    bitmap[byte_index] &= ~(1 << bit_index);
}

void mark_block_free(void *disk, int block_num) {
    uint8_t *bitmap = (uint8_t *)disk + BLOCK_SIZE;
    int byte_index = block_num / 8;
    int bit_index = block_num % 8;
    bitmap[byte_index] |= (1 << bit_index);
}

int find_free_block(void *disk) {
    uint8_t *bitmap = (uint8_t *)disk + BLOCK_SIZE;
    for (int i = 0; i < BITMAP_BYTES; i++) {
        if (bitmap[i] != 0) {
            for (int j = 0; j < 8; j++) {
                if (bitmap[i] & (1 << j)) {
                    return i * 8 + j;
                }
            }
        }
    }
    return -1;  // No free blocks
}

int is_block_free(void *disk, int block_num) {
    uint8_t *bitmap = (uint8_t *)disk + BLOCK_SIZE;
    int byte_index = block_num / 8;
    int bit_index = block_num % 8;
    return (bitmap[byte_index] & (1 << bit_index)) != 0;
}