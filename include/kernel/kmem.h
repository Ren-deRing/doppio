#pragma once

#include <stdint.h>
#include <stddef.h>

#include <kernel/lock.h>
#include <kernel/mmu.h>

#define KMEM_NUM_CLASSES  16
#define KMEM_MAG_CAPACITY 16
#define KMEM_MAX_DIRECT   2048

typedef struct kmem_magazine {
    struct kmem_magazine* next;
    void* slots[KMEM_MAG_CAPACITY];
    int top;
} kmem_magazine_t;

typedef struct kmem_depot {
    spinlock_t lock;
    
    kmem_magazine_t* full_mags;
    kmem_magazine_t* empty_mags;
    uint32_t full_count;
    uint32_t empty_count;

    uintptr_t current_chunk;
    uint32_t  objs_remaining;

    uint8_t   current_order;
} kmem_depot_t;

void* kmalloc(size_t size);
void  kfree(void* ptr);
void* kcalloc(size_t nmemb, size_t size);
void* krealloc(void* ptr, size_t size);
void* kmemalign(size_t alignment, size_t size);

void* kmalloc_aligned(size_t size, size_t alignment);
void  kfree_aligned(void* ptr);

void  kmem_init(void);