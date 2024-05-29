#include "my_malloc.h"
#include <limits.h>
#include <stdint.h>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t nolock = PTHREAD_MUTEX_INITIALIZER;

__thread Block *head_tls = NULL;
__thread Block *tail_tls = NULL; 

Block *head = NULL;
Block *tail = NULL;

unsigned long heap = 0;
unsigned long free_size = 0;

// void printlist() {
//     printf("start\n");
//     Block * tmp = head;
//     if (tmp) {
//         printf("Addr: %p, Size: %ld\n", (void *)tmp, tmp->size);
//         //printf("sizeof Block:\n");
//         //printf("%ld", sizeof(Block));
//         while (tmp->next) {
//             tmp = tmp->next;
//             printf("Addr: %p, Size: %ld\n", (void *)tmp, tmp->size);
//         }
//     }
// }

void *ts_malloc_lock(size_t size){
    pthread_mutex_lock(&lock);
    void *ptr = bf_malloc(size);
    pthread_mutex_unlock(&lock);
    return ptr;
}

void ts_free_lock(void *ptr) {
    pthread_mutex_lock(&lock);
    bf_free(ptr);
    pthread_mutex_unlock(&lock);
}

void *ts_malloc_nolock(size_t size) {
    void *ptr = bf_malloc_tls(size);
    return ptr;
}

void ts_free_nolock(void *ptr) {
    bf_free_tls(ptr);
}

Block *findBest(size_t size){
    Block *curr = head;
    size_t min_size_diff = SIZE_MAX;
    Block *best = NULL;

    while (curr) {
        if (curr->size > size) {
            size_t size_diff = curr->size - size;
            if (size_diff < min_size_diff) {
                min_size_diff = size_diff;
                best = curr;
            }
        }else if(curr->size == size){
            best = curr;
            break;
        }
        curr = curr->next;
    }

    return best;
}

Block *findBest_tls(size_t size){
    Block *curr = head_tls;
    size_t min_size_diff = SIZE_MAX;
    Block *best = NULL;

    while (curr) {
        if (curr->size > size) {
            size_t size_diff = curr->size - size;
            if (size_diff < min_size_diff) {
                min_size_diff = size_diff;
                best = curr;
            }
        }else if(curr->size == size){
            best = curr;
            break;
        }
        curr = curr->next;
    }

    return best;
}

void *bf_malloc_tls(size_t size) {
    Block *curr = findBest_tls(size);

    if (curr) { // block found
        if (curr->size > size + sizeof(Block)) {
            return splitBlock_tls(curr, size);
        } else {
            return useBlock_tls(curr);
        }
    }else{ //curr is NULL, extend memory
        return extendBlock(size);
    }
}

void *bf_malloc(size_t size) {
    Block *curr = findBest(size);

    if (curr) { // block found
        if (curr->size > size + sizeof(Block)) {
            return splitBlock(curr, size);
        } else {
            return useBlock(curr);
        }
    }else{ //curr is NULL, extend memory
        return extendBlock(size);
    }
}

// void *tls_malloc(size_t size, Block * head, Block * tail) {
//     Block *curr = findBest(size, head);

//     if (curr) { // block found
//         if (curr->size > size + sizeof(Block)) {
//             return splitBlock(curr, size, head, tail);
//         } else {
//             return useBlock(curr, head, tail);
//         }
//     }else{ //curr is NULL, extend memory
//         return extendBlock(size);
//     }
// }


void *splitBlock(Block * block, size_t size) {
    Block * new = (Block *)((void *)block + sizeof(Block) + size);
    new->size = block->size - size - sizeof(Block);
    new->isFree = 1;
    replaceBlock(new, block);

    block->size = size;
    block->isFree = 0;
    block->prev = NULL;
    block->next = NULL;

    free_size -= (size + sizeof(Block));

    return (void *)((char *)block + sizeof(Block));
}

void *splitBlock_tls(Block * block, size_t size) {
    Block * new = (Block *)((void *)block + sizeof(Block) + size);
    new->size = block->size - size - sizeof(Block);
    new->isFree = 1;
    replaceBlock_tls(new, block);

    block->size = size;
    block->isFree = 0;
    block->prev = NULL;
    block->next = NULL;

    free_size -= (size + sizeof(Block));

    return (void *)((char *)block + sizeof(Block));
}

void *useBlock(Block * block) {
    if (block->next){
        if(block->prev){ // middle
            block->next->prev = block->prev;
            block->prev->next = block->next;
        }else{ // head
            head = block->next;
            head->prev = NULL;
        }
    }else{
        if(block->prev){ // tail
            tail = block->prev;
            tail->next = NULL;
        }else{ // only one
            head = NULL;
            tail = NULL;
        }
    }
    block->prev = NULL;
    block->next = NULL;
    
    free_size -= (block->size + sizeof(Block));

    return (void *)((char *)block + sizeof(Block));
}

void *useBlock_tls(Block * block) {
    if (block->next){
        if(block->prev){ // middle
            block->next->prev = block->prev;
            block->prev->next = block->next;
        }else{ // head
            head_tls = block->next;
            head_tls->prev = NULL;
        }
    }else{
        if(block->prev){ // tail
            tail_tls = block->prev;
            tail_tls->next = NULL;
        }else{ // only one
            head_tls = NULL;
            tail_tls = NULL;
        }
    }
    block->prev = NULL;
    block->next = NULL;
    
    free_size -= (block->size + sizeof(Block));

    return (void *)((char *)block + sizeof(Block));
}

void *extendBlock(size_t size) {
    pthread_mutex_lock(&nolock);
    Block * new = (Block *)sbrk(size + sizeof(Block)); // new allocated block address
    pthread_mutex_unlock(&nolock);
    if (new == (void *)-1) { // failed
        return NULL;
    }
    new->size = size;
    new->isFree = 0;
    new->prev = NULL;
    new->next = NULL;
    heap += size + sizeof(Block);
    return (void *)((char *)new + sizeof(Block));
}

// void *extendWithLock(size_t size){
//     pthread_mutex_lock(&lock);
//     Block * new = (Block *)sbrk(size + sizeof(Block));
//     pthread_mutex_unlock(&lock);
//     if (new == (void *)-1) { // failed
//         return NULL;
//     }
//     new->size = size;
//     new->isFree = 0;
//     new->prev = NULL;
//     new->next = NULL;
//     heap += size + sizeof(Block);
//     return (void *)((char *)new + sizeof(Block));
// }

void replaceBlock_tls(Block * new, Block * old){
    new->prev = old->prev;
    new->next = old->next;

    if (old->prev) {
        old->prev->next = new;
    } else {
        head_tls = new;
    }

    if (old->next) {
        old->next->prev = new;
    } else {
        tail_tls = new;  
    }
}


void replaceBlock(Block * new, Block * old){
    new->prev = old->prev;
    new->next = old->next;

    if (old->prev) {
        old->prev->next = new;
    } else {
        head = new;
    }

    if (old->next) {
        old->next->prev = new;
    } else {
        tail = new;  
    }
}

void bf_free(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    Block * curr = (Block *)(ptr - sizeof(Block));
    curr->isFree = 1;

    if (head == NULL && tail == NULL) { // single one
        head = curr;
        tail = curr;
        curr->prev = NULL;
        curr->next = NULL;
    } else {
        Block *current = head;
        while (current != NULL && current < curr) {
            current = current->next;
        }
        if (current == head) {
            // head
            curr->next = head;
            curr->prev = NULL;
            head->prev = curr;
            head = curr;
        } else if (current == NULL) {
            // tail
            tail->next = curr;
            curr->prev = tail;
            curr->next = NULL;
            tail = curr;
        } else {
            // middle
            curr->next = current;
            curr->prev = current->prev;
            current->prev->next = curr;
            current->prev = curr;
        }
    }

    free_size += curr->size + sizeof(Block);

    if (curr->prev){
        Block *first = curr->prev;
        if(((char *)first + first->size + sizeof(Block)) == (char *)curr) { // need to merge
            first->size += curr->size + sizeof(Block);
            first->next = curr->next;
            if (curr->next) {
                curr->next->prev = first;
            } else {
                tail = first;
            }
            curr = first;
        }
    } 

     if (curr->next){
        Block *second = curr->next;
        if(((char *)curr + curr->size + sizeof(Block)) == (char *)second) { // need to merge
            curr->size += second->size + sizeof(Block);
            curr->next = second->next;
            if (curr->next) {
                curr->next->prev = curr;
            } else {
                tail = curr;
            }
        }
    } 
    //printlist();
}

void bf_free_tls(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    Block * curr = (Block *)(ptr - sizeof(Block));
    curr->isFree = 1;

    if (head_tls == NULL && tail_tls == NULL) { // single one
        head_tls = curr;
        tail_tls = curr;
        curr->prev = NULL;
        curr->next = NULL;
    } else {
        Block *current = head_tls;
        while (current != NULL && current < curr) {
            current = current->next;
        }
        if (current == head_tls) {
            // head
            curr->next = head_tls;
            curr->prev = NULL;
            head_tls->prev = curr;
            head_tls = curr;
        } else if (current == NULL) {
            // tail
            tail_tls->next = curr;
            curr->prev = tail_tls;
            curr->next = NULL;
            tail_tls = curr;
        } else {
            // middle
            curr->next = current;
            curr->prev = current->prev;
            current->prev->next = curr;
            current->prev = curr;
        }
    }

    free_size += curr->size + sizeof(Block);

    if (curr->prev){
        Block *first = curr->prev;
        if(((char *)first + first->size + sizeof(Block)) == (char *)curr) { // need to merge
            first->size += curr->size + sizeof(Block);
            first->next = curr->next;
            if (curr->next) {
                curr->next->prev = first;
            } else {
                tail_tls = first;
            }
            curr = first;
        }
    } 

     if (curr->next){
        Block *second = curr->next;
        if(((char *)curr + curr->size + sizeof(Block)) == (char *)second) { // need to merge
            curr->size += second->size + sizeof(Block);
            curr->next = second->next;
            if (curr->next) {
                curr->next->prev = curr;
            } else {
                tail_tls = curr;
            }
        }
    }
    //printlist();
}

