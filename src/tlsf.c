#include <unistd.h>
#include <syscall.h>
#include <sys/mman.h>
#include "tlsf.h"

#define PAGE_SIZE (4096)

uint32_t
round_up(uint32_t a,uint32_t b){
  return (a+b-1)&(~(b-1));
}

/* flsl(0) -> -1
 * flsl(1) -> 0
 * flsl(2) -> 1
 * flsl(3) -> 1
 * flsl(4) -> 2
 */

int8_t
flsl(uint32_t x){
  int8_t n = -1;
  if(x&0xFFFF0000){n+=16;x>>=16;}
  if(x&0xFF00){n+=8;x>>=8;}
  if(x&0xF0){n+=4;x>>=4;}
  if(x&0xC){n+=2;x>>=2;}
  if(x&0x2){n+=1;x>>=1;}
  return n+x;
}

struct index{
  int8_t fl,sl;
} __attribute__((packed));

struct index
mapping_insert(uint32_t nwords){
  int8_t fli = flsl(nwords>>3);
  struct index idx = {
    .fl = fli + 1,
    .sl = (nwords>>((fli<0)?0:fli))&0x7,
  };
  return idx;
}

uint32_t
mapping_roundup(uint32_t nwords){
  int fli = flsl(nwords>>3);
  return round_up(nwords,(fli<0)?1:(1<<fli));
}

struct index
mapping_search(uint32_t nwords){
  return mapping_insert(mapping_roundup(nwords));
}


struct free_block*
find_suitable_block(struct memory_pool*pool,uint32_t nwords){
  struct index i = mapping_search(nwords);

  if(pool->fl_bitmap & (1 << i.fl)) {
    int8_t slb = flsl(pool->sl_bitmap[i.fl] >> i.sl);
    if(slb>=0) {
      return pool->free_block_list[i.fl][i.sl+slb];
    }
  }

  int8_t flb = flsl(pool->fl_bitmap >> (i.fl + 1));
  if(flb < 0)
    return NULL;
  int8_t fli = i.fl+1+flb;
  int8_t sli = flsl(pool->sl_bitmap[fli]);
  return pool->free_block_list[fli][sli];
}

void
insert_free_block(struct memory_pool*pool,struct free_block*block){
  block->header.free = 1;
  block->prev_block = NULL;
  pool->free_size += block->header.size;
  pool->free_blocks += 1;

  struct index i = mapping_insert(block->header.size);
  block->next_block = pool->free_block_list[i.fl][i.sl];

  if(block->next_block)
    block->next_block->prev_block = block;

  pool->free_block_list[i.fl][i.sl] = block;
  pool->sl_bitmap[i.fl] |= 1 << (i.sl);
  pool->fl_bitmap |= 1 << (i.fl);
}


void
remove_free_block(struct memory_pool*pool,struct free_block*block){
  block->header.free = 0;
  pool->free_size -= block->header.size;
  pool->free_blocks -= 1;

  if(block->next_block)
    block->next_block->prev_block = block->prev_block;

  if(block->prev_block){
    block->prev_block->next_block = block->next_block;
    return;
  }

  struct index i= mapping_insert(block->header.size);
  pool->free_block_list[i.fl][i.sl] = block->next_block;

  if(block->next_block) return;
  pool->sl_bitmap[i.fl] &= ~(1U << i.sl);
  if (pool->sl_bitmap[i.fl]) return;
  pool->fl_bitmap &= ~(1U << i.fl);
}

struct block_header *
find_left_block(struct block_header *header) {
  return (struct block_header *)((uintptr_t *)header - header->prev_size) - 1;
}

struct block_header *
find_right_block(struct block_header *header) {
  return (struct block_header *)((uintptr_t *)(header+1) + header->size);
}

void
split_block(struct memory_pool*pool,struct block_header *block,uint32_t nwords) {
  if (sizeof(uintptr_t) * (block->size - nwords) < sizeof(struct free_block))
    return;

  uint32_t remain_size = block->size - nwords - sizeof(struct block_header)/sizeof(uintptr_t);
  block->size = nwords;
  struct block_header *remain_block = find_right_block(block);
  remain_block->prev_size = nwords;
  remain_block->size = remain_size;
  find_right_block(remain_block)->prev_size = remain_size;

  insert_free_block(pool,(struct free_block*)remain_block);
}

struct block_header *
merge_left_block(struct memory_pool*pool,struct block_header *block) {
  struct block_header *left_block = find_left_block(block);
  if(!(left_block->free))
    return block;

  remove_free_block(pool,(struct free_block *)left_block);
  left_block->size += block->size + sizeof(struct block_header)/sizeof(uintptr_t);

  find_right_block(left_block)->prev_size = left_block->size;
  return left_block;
}

void
merge_right_block(struct memory_pool*pool,struct block_header *block) {
  struct block_header *right_block = find_right_block(block);
  if(!(right_block->free))
    return;

  remove_free_block(pool,(struct free_block *)right_block);
  block->size += right_block->size + sizeof(struct block_header)/sizeof(uintptr_t);

  find_right_block(block)->prev_size = block->size;
}

struct block_header *
merge_block(struct memory_pool*pool,struct block_header *block) {
  block = merge_left_block(pool,block);
  merge_right_block(pool,block);
  return block;
}


struct block_header*
find_first_block(struct memory_pool*pool){
  return (struct block_header*)(pool->base);
}

struct block_header*
find_last_block(struct memory_pool*pool){
  return (struct block_header*)(pool->base+pool->pages*PAGE_SIZE)-1;
}

bool
alloc_page(void *addr, size_t length) {
  return (void*)syscall(SYS_mmap,
                        addr,
                        length,
                        PROT_READ|PROT_WRITE,
                        MAP_ANONYMOUS|MAP_PRIVATE|MAP_FIXED,
                        -1, 0) != MAP_FAILED;
}

void
mem_init(struct memory_pool*pool,void *base){
  pool->base = base;
  pool->pages = 1;
  alloc_page(base,PAGE_SIZE);
  struct block_header*first_block = find_first_block(pool);
  struct block_header*last_block = find_last_block(pool);
  uint32_t block_size = (PAGE_SIZE-sizeof(struct block_header)*3)/sizeof(uintptr_t);

  first_block->prev_size=0;
  first_block->size=0;
  first_block->free=0;

  last_block->prev_size=block_size;
  last_block->size=0;
  last_block->free=0;

  struct block_header*free_block = find_right_block(first_block);
  free_block->prev_size = 0;
  free_block->size = block_size;
  insert_free_block(pool,(struct free_block*)free_block);
}

void
mem_free(struct memory_pool*pool,void *ptr){
  if (!ptr)
    return;

  struct block_header *header = merge_block(pool,(struct block_header *)ptr - 1);

  insert_free_block(pool,(struct free_block*)header);
  pool->used_blocks -= 1;
}


void*
mem_alloc(struct memory_pool*pool,uint32_t size,uint8_t tag,bool grow){
  if(!size)
    return NULL;

  uint32_t nwords=round_up(size,sizeof(uintptr_t))/sizeof(uintptr_t);
  struct free_block*free_block = find_suitable_block(pool,nwords);

  if(!free_block){
    if(!grow)
      return NULL;

    void *alloc_base = pool->base + PAGE_SIZE*pool->pages;
    size_t length = mapping_roundup(nwords)*sizeof(uintptr_t) + sizeof(struct block_header);

    struct block_header *last_block = find_last_block(pool);
    struct block_header *left_block = find_left_block(last_block);

    if(left_block->free)
      length -= sizeof(struct block_header)+left_block->size*sizeof(uintptr_t);

    length = round_up(length,PAGE_SIZE);
    if(alloc_page(alloc_base,length)){
      pool->pages += length/PAGE_SIZE;

      if(left_block->free){
        remove_free_block(pool,(struct free_block*)left_block);
        left_block->size += length/sizeof(uintptr_t);
        free_block = (struct free_block*)left_block;
      }else{
        last_block->size = (length-sizeof(struct block_header))/sizeof(uintptr_t);
        free_block = (struct free_block*)last_block;
      }

      last_block = find_last_block(pool);
      last_block->prev_size = free_block->header.size;
      last_block->size = 0;
      last_block->free = 0;
    }
  }else{
    remove_free_block(pool,free_block);
  }

  if(!free_block)
    return NULL;

  split_block(pool,&(free_block->header),nwords);
  free_block->header.marked = 0;
  free_block->header.tag = tag;
  pool->used_blocks += 1;
  return (void*)(&(free_block->header)+1);
}

uint8_t
mem_tag(void*ptr){
  return ((struct block_header*)ptr-1)->tag;
}
