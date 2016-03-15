#pragma once
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct block_header {
  unsigned int prev_size : 24;
  unsigned int size      : 24;
  unsigned int free      : 1;
  unsigned int marked    : 1;
  unsigned int unused    : 6;
  unsigned int tag       : 8;
} __attribute__((packed));

struct free_block {
  struct block_header header;
  struct free_block *prev_block;
  struct free_block *next_block;
};

/* fli number of words
 *   6 256
 *   5 128
 *   4 64
 *   3 32
 *   2 16 +0 +2 +4 +6 +8 +10 +12 +14
 *   1  8 +0 +1 +2 +3 +4  +5  +6  +7
 *   0  0 +0 +1 +2 +3 +4  +5  +6  +7
 */

struct memory_pool {
  uint32_t pages,fl_bitmap,free_size,free_blocks,used_blocks;
  uint8_t sl_bitmap[24];
  struct free_block *free_block_list[24][8];
  void *base;
} __attribute__((packed));

void
mem_init(struct memory_pool*,void*base);

void *
mem_alloc(struct memory_pool*,uint32_t n,uint8_t tag,bool grow);

void
mem_free(struct memory_pool*,void*ptr);

uint8_t
mem_tag(void*ptr);
