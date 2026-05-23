#ifndef TREE_ALLOCATOR_H
#define TREE_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tree_allocator {
    void* (*malloc_fn)(size_t size, void* ctx);
    void (*free_fn)(void* ptr, void* ctx);
    void* ctx;
    bool owns_ctx;
    size_t allocation_count;
    size_t allocated_bytes;
    void (*leak_hook)(const void* ptr, size_t size, void* ctx);
} tree_allocator_t;

typedef struct tree_pool_allocator_params {
    size_t block_count;
    size_t node_size;
} tree_pool_allocator_params_t;

void tree_allocator_default_init(tree_allocator_t* allocator);
int tree_allocator_pool_init(tree_allocator_t* allocator, const tree_pool_allocator_params_t* params);
void tree_allocator_destroy(tree_allocator_t* allocator);

size_t tree_allocator_allocated_bytes(const tree_allocator_t* allocator);
size_t tree_allocator_allocation_count(const tree_allocator_t* allocator);

#ifdef __cplusplus
}
#endif

#endif /* TREE_ALLOCATOR_H */
