#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Block {
  size_t size;
  int isFree;
  struct Block * prev;
  struct Block * next;
} Block;

//Thread Safe malloc/free: locking version 
void *ts_malloc_lock(size_t size);
void ts_free_lock(void *ptr);

//Thread Safe malloc/free: non-locking version 
void *ts_malloc_nolock(size_t size);
void ts_free_nolock(void *ptr);


// Best Fit
void *bf_malloc(size_t size);
void bf_free(void * ptr);
//void *tls_malloc(size_t size, Block * head, Block * tail);
void *splitBlock(Block * block, size_t size);
void *useBlock(Block * block);
void *extendBlock(size_t size);
Block *findBest(size_t size);
//void *extendWithLock(size_t size);
void replaceBlock(Block * new, Block * old);
Block *findBest_tls(size_t size);
void *bf_malloc_tls(size_t size);
void *splitBlock_tls(Block * block, size_t size);
void *useBlock_tls(Block * block);
void replaceBlock_tls(Block * new, Block * old);
void bf_free_tls(void * ptr);


//void printlist();

#endif // MY_MALLOC_H