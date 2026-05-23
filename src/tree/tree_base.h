#ifndef TREE_BASE_H
#define TREE_BASE_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TREE_LIKELY(x) __builtin_expect(!!(x), 1)
#define TREE_UNLIKELY(x) __builtin_expect(!!(x), 0)

#include "allocator.h"

typedef enum tree_status {
    TREE_OK = 0,
    TREE_ERR_OOM,
    TREE_ERR_DUPLICATE,
    TREE_ERR_NOT_FOUND,
    TREE_ERR_INVALID_ARGUMENT,
    TREE_ERR_UNSUPPORTED,
    TREE_ERR_CORRUPTED,
    TREE_ERR_IO,
    TREE_ERR_LOCKED,
} tree_status_t;

typedef enum tree_color {
    RB_RED = 0,
    RB_BLACK = 1,
} tree_color_t;

typedef enum tree_type {
    TREE_TYPE_BINARY = 0,
    TREE_TYPE_BST,
    TREE_TYPE_AVL,
    TREE_TYPE_RB,
} tree_type_t;

typedef int (*tree_compare_fn)(const void* lhs, const void* rhs, void* user_ctx);

typedef tree_status_t (*tree_copy_fn)(const void* src, void* dst, size_t size, void* user_ctx);

typedef void (*tree_destroy_fn)(void* value, void* user_ctx);

typedef void (*tree_print_fn)(const void* value, void* user_ctx, char* buffer, size_t size);

typedef tree_status_t (*tree_serialize_fn)(const void* value, void* buffer, size_t buffer_size, void* user_ctx);

typedef tree_status_t (*tree_deserialize_fn)(const void* buffer, size_t buffer_size, void* out_value, void* user_ctx);

typedef struct tree_config {
    tree_compare_fn compare;
    tree_copy_fn copy;
    tree_destroy_fn destroy;
    tree_print_fn print;
    tree_serialize_fn serialize;
    tree_deserialize_fn deserialize;
    void* user_ctx;
} tree_config_t;

typedef struct tree_node {
    struct tree_node* left;
    struct tree_node* right;
    struct tree_node* parent;
    int height;
    tree_color_t color;
    uint8_t payload[];
} tree_node_t;

typedef struct tree_t {
    tree_node_t* root;
    tree_node_t* nil;
    tree_type_t type;
    tree_config_t config;
    tree_allocator_t allocator;
    size_t element_size;
    size_t count;
    bool owns_allocator;
    void* lock_ctx;
} tree_t;

typedef int (*tree_visit_fn)(const void* value, void* user_ctx);

typedef struct tree_iterator tree_iterator_t;

/* Base tree API */

void tree_allocator_default_init(tree_allocator_t* allocator);

tree_status_t tree_init(tree_t* tree, tree_type_t type, const tree_config_t* config, size_t element_size);
tree_status_t tree_destroy(tree_t* tree);
tree_status_t tree_clear(tree_t* tree);
size_t tree_size(const tree_t* tree);
bool tree_is_empty(const tree_t* tree);

tree_node_t* tree_root(const tree_t* tree);

tree_node_t* tree_node_create(tree_t* tree, const void* value);
void tree_node_destroy(tree_t* tree, tree_node_t* node);
const void* tree_node_value(const tree_node_t* node);

int tree_height(const tree_t* tree);
int tree_depth(const tree_t* tree, const tree_node_t* node);
size_t tree_node_count(const tree_t* tree);
size_t tree_leaf_count(const tree_t* tree);

tree_status_t tree_traverse_preorder(const tree_t* tree, tree_visit_fn visit, void* user_ctx);
tree_status_t tree_traverse_inorder(const tree_t* tree, tree_visit_fn visit, void* user_ctx);
tree_status_t tree_traverse_postorder(const tree_t* tree, tree_visit_fn visit, void* user_ctx);
tree_status_t tree_traverse_level_order(const tree_t* tree, tree_visit_fn visit, void* user_ctx);

/* Generic dispatch wrappers for ordered tree operations */

tree_status_t tree_insert(tree_t* tree, const void* value);
tree_status_t tree_remove(tree_t* tree, const void* key);
const void* tree_search(const tree_t* tree, const void* key);
bool tree_contains(const tree_t* tree, const void* key);

/* Typed API generation */

#define TREE_DECLARE(name, element_type)                                      \
    typedef struct name##_t { tree_t base; } name##_t;                         \
    static inline tree_status_t name##_init(name##_t* tree,                    \
                                           tree_type_t type,                   \
                                           const tree_config_t* cfg) {         \
        return tree_init(&tree->base, type, cfg, sizeof(element_type));        \
    }                                                                         \
    static inline tree_status_t name##_destroy(name##_t* tree) {              \
        return tree_destroy(&tree->base);                                     \
    }                                                                         \
    static inline size_t name##_size(const name##_t* tree) {                  \
        return tree_size(&tree->base);                                        \
    }                                                                         \
    static inline bool name##_is_empty(const name##_t* tree) {                \
        return tree_is_empty(&tree->base);                                    \
    }                                                                         \
    static inline tree_status_t name##_insert(name##_t* tree,                  \
                                             const element_type* value) {      \
        return tree_insert(&tree->base, value);                               \
    }                                                                         \
    static inline tree_status_t name##_remove(name##_t* tree,                  \
                                             const element_type* key) {        \
        return tree_remove(&tree->base, key);                                 \
    }                                                                         \
    static inline const element_type* name##_search(const name##_t* tree,      \
                                           const element_type* key) {          \
        return (const element_type*)tree_search(&tree->base, key);            \
    }

#define TREE_DEFINE(name, type)

#ifdef __cplusplus
}
#endif

#endif /* TREE_BASE_H */
