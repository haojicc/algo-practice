#include "allocator.h"
#include <stdlib.h>
#include <string.h>

typedef struct tree_pool_block {
    struct tree_pool_block* next;
    size_t used;
    size_t capacity;
    uint8_t data[];
} tree_pool_block_t;

typedef struct tree_pool_context {
    tree_pool_block_t* head;
    size_t block_count;
    size_t node_size;
} tree_pool_context_t;

static void* pool_malloc(size_t size, void* context) {
    tree_pool_context_t* pool = (tree_pool_context_t*)context;
    if (pool == NULL || size == 0) {
        return NULL;
    }

    if (size > pool->node_size) {
        return NULL;
    }

    tree_pool_block_t* block = pool->head;
    while (block != NULL) {
        if (block->capacity - block->used >= size) {
            void* memory = block->data + block->used;
            block->used += size;
            return memory;
        }
        block = block->next;
    }

    size_t capacity = pool->node_size * pool->block_count;
    tree_pool_block_t* new_block = (tree_pool_block_t*)malloc(sizeof(tree_pool_block_t) + capacity);
    if (new_block == NULL) {
        return NULL;
    }
    new_block->next = pool->head;
    new_block->used = size;
    new_block->capacity = capacity;
    pool->head = new_block;
    return new_block->data;
}

static void* default_malloc(size_t size, void* context) {
    (void)context;
    return malloc(size);
}

static void default_free(void* ptr, void* context) {
    (void)context;
    free(ptr);
}

static void pool_free(void* ptr, void* context) {
    (void)ptr;
    (void)context;
}

void tree_allocator_default_init(tree_allocator_t* allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->malloc_fn = default_malloc;
    allocator->free_fn = default_free;
    allocator->ctx = NULL;
    allocator->allocation_count = 0;
    allocator->allocated_bytes = 0;
    allocator->leak_hook = NULL;
}

int tree_allocator_pool_init(tree_allocator_t* allocator, const tree_pool_allocator_params_t* params) {
    if (allocator == NULL || params == NULL || params->block_count == 0 || params->node_size == 0) {
        return -1;
    }

    tree_pool_context_t* context = (tree_pool_context_t*)malloc(sizeof(tree_pool_context_t));
    if (context == NULL) {
        return -1;
    }

    context->head = NULL;
    context->block_count = params->block_count;
    context->node_size = params->node_size;

    allocator->malloc_fn = pool_malloc;
    allocator->free_fn = pool_free;
    allocator->ctx = context;
    allocator->owns_ctx = true;
    allocator->allocation_count = 0;
    allocator->allocated_bytes = 0;
    allocator->leak_hook = NULL;
    return 0;
}

void tree_allocator_destroy(tree_allocator_t* allocator) {
    if (allocator == NULL) {
        return;
    }

    if (allocator->owns_ctx) {
        tree_pool_context_t* context = (tree_pool_context_t*)allocator->ctx;
        if (context != NULL) {
            tree_pool_block_t* block = context->head;
            while (block != NULL) {
                tree_pool_block_t* next = block->next;
                free(block);
                block = next;
            }
            free(context);
        }
    }
    allocator->ctx = NULL;
    allocator->malloc_fn = NULL;
    allocator->free_fn = NULL;
}

size_t tree_allocator_allocated_bytes(const tree_allocator_t* allocator) {
    return allocator ? allocator->allocated_bytes : 0;
}

size_t tree_allocator_allocation_count(const tree_allocator_t* allocator) {
    return allocator ? allocator->allocation_count : 0;
}
