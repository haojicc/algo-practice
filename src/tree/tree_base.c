#include "tree_base.h"
#include <stdlib.h>
#include <string.h>
#include "allocator.h"
#include "iterator.h"
#include "binary_tree.h"
#include "bst.h"
#include "avl_tree.h"
#include "rb_tree.h"

tree_node_t* tree_node_create(tree_t* tree, const void* value) {
    if (TREE_UNLIKELY(tree == NULL || value == NULL)) {
        return NULL;
    }

    size_t size = sizeof(tree_node_t) + tree->element_size;
    tree_node_t* node = tree->allocator.malloc_fn(size, tree->allocator.ctx);
    if (TREE_UNLIKELY(node == NULL)) {
        return NULL;
    }

    node->left = node->right = node->parent = NULL;
    node->height = 1;
    node->color = RB_RED;
    if (tree->config.copy) {
        if (tree->config.copy(value, node->payload, tree->element_size, tree->config.user_ctx) != TREE_OK) {
            tree->allocator.free_fn(node, tree->allocator.ctx);
            return NULL;
        }
    } else {
        memcpy(node->payload, value, tree->element_size);
    }

    return node;
}

static tree_status_t tree_default_copy(const void* src, void* dst, size_t size, void* user_ctx) {
    (void)user_ctx;
    if (src == NULL || dst == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    memcpy(dst, src, size);
    return TREE_OK;
}

static void tree_node_release(tree_t* tree, tree_node_t* node) {
    if (TREE_UNLIKELY(tree == NULL || node == NULL)) {
        return;
    }

    if (tree->config.destroy) {
        tree->config.destroy(node->payload, tree->config.user_ctx);
    }

    tree->allocator.free_fn(node, tree->allocator.ctx);
}

void tree_node_destroy(tree_t* tree, tree_node_t* node) {
    tree_node_release(tree, node);
}

static void* tree_allocator_malloc(size_t size, void* ctx) {
    tree_allocator_t* allocator = (tree_allocator_t*)ctx;
    void* data = malloc(size);
    if (data != NULL) {
        allocator->allocation_count += 1;
        allocator->allocated_bytes += size;
        if (allocator->leak_hook) {
            allocator->leak_hook(data, size, allocator->ctx);
        }
    }
    return data;
}

static void tree_allocator_free(void* ptr, void* ctx) {
    tree_allocator_t* allocator = (tree_allocator_t*)ctx;
    if (ptr == NULL) {
        return;
    }
    free(ptr);
    if (allocator->leak_hook) {
        allocator->leak_hook(ptr, 0, allocator->ctx);
    }
}

static tree_status_t tree_allocators_prepare(tree_t* tree, const tree_config_t* config) {
    if (tree == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    if (config == NULL || config->compare == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree_allocator_default_init(&tree->allocator);
    tree->owns_allocator = true;
    tree->config = *config;
    if (tree->config.copy == NULL) {
        tree->config.copy = tree_default_copy;
    }
    if (tree->config.destroy == NULL) {
        tree->config.destroy = NULL;
    }
    if (tree->config.print == NULL) {
        tree->config.print = NULL;
    }
    if (tree->config.serialize == NULL) {
        tree->config.serialize = NULL;
    }
    if (tree->config.deserialize == NULL) {
        tree->config.deserialize = NULL;
    }

    tree->allocator.malloc_fn = tree_allocator_malloc;
    tree->allocator.free_fn = tree_allocator_free;
    tree->allocator.ctx = &tree->allocator;
    tree->allocator.leak_hook = NULL;
    tree->allocator.allocation_count = 0;
    tree->allocator.allocated_bytes = 0;

    return TREE_OK;
}

static void tree_release_node_recursive(tree_t* tree, tree_node_t* node) {
    if (node == NULL || node == tree->nil) {
        return;
    }
    tree_release_node_recursive(tree, node->left);
    tree_release_node_recursive(tree, node->right);
    tree_node_release(tree, node);
}

static void tree_release_nodes(tree_t* tree) {
    if (tree == NULL) {
        return;
    }
    if (tree->type == TREE_TYPE_RB) {
        if (tree->root != NULL && tree->root != tree->nil) {
            tree_release_node_recursive(tree, tree->root);
        }
        if (tree->nil) {
            tree->allocator.free_fn(tree->nil, tree->allocator.ctx);
            tree->nil = NULL;
        }
    } else {
        tree_release_node_recursive(tree, tree->root);
    }
}

tree_status_t tree_init(tree_t* tree, tree_type_t type, const tree_config_t* config, size_t element_size) {
    if (tree == NULL || config == NULL || config->compare == NULL || element_size == 0) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree->root = NULL;
    tree->nil = NULL;
    tree->type = type;
    tree->element_size = element_size;
    tree->count = 0;
    tree->lock_ctx = NULL;

    tree_status_t status = tree_allocators_prepare(tree, config);
    if (status != TREE_OK) {
        return status;
    }

    if (type == TREE_TYPE_RB) {
        size_t header_size = sizeof(tree_node_t);
        tree->nil = tree->allocator.malloc_fn(header_size, tree->allocator.ctx);
        if (tree->nil == NULL) {
            return TREE_ERR_OOM;
        }
        tree->nil->left = tree->nil->right = tree->nil->parent = tree->nil;
        tree->nil->height = 0;
        tree->nil->color = RB_BLACK;
        tree->root = tree->nil;
    }

    return TREE_OK;
}

tree_status_t tree_destroy(tree_t* tree) {
    if (tree == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree_release_nodes(tree);
    if (tree->owns_allocator) {
        tree_allocator_destroy(&tree->allocator);
    }
    tree->root = NULL;
    tree->nil = NULL;
    tree->count = 0;
    return TREE_OK;
}

static tree_node_t* tree_root_node(const tree_t* tree) {
    if (tree == NULL) {
        return NULL;
    }
    if (tree->type == TREE_TYPE_RB) {
        return tree->root == tree->nil ? NULL : tree->root;
    }
    return tree->root;
}

tree_status_t tree_clear(tree_t* tree) {
    if (tree == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree_release_nodes(tree);
    tree->count = 0;
    if (tree->type == TREE_TYPE_RB) {
        tree->root = tree->nil;
    } else {
        tree->root = NULL;
    }
    return TREE_OK;
}

size_t tree_size(const tree_t* tree) {
    return tree ? tree->count : 0;
}

bool tree_is_empty(const tree_t* tree) {
    if (tree == NULL) {
        return true;
    }
    if (tree->type == TREE_TYPE_RB) {
        return tree->root == tree->nil || tree->root == NULL;
    }
    return tree->root == NULL;
}

tree_node_t* tree_root(const tree_t* tree) {
    return tree_root_node(tree);
}

const void* tree_node_value(const tree_node_t* node) {
    return node ? node->payload : NULL;
}

static int tree_node_height(const tree_node_t* node) {
    if (node == NULL) {
        return 0;
    }
    return node->height;
}

int tree_height(const tree_t* tree) {
    if (tree == NULL) {
        return 0;
    }
    tree_node_t* root = tree_root_node(tree);
    return root ? root->height : 0;
}

int tree_depth(const tree_t* tree, const tree_node_t* node) {
    if (tree == NULL || node == NULL) {
        return -1;
    }
    int depth = 0;
    const tree_node_t* iterator = node;
    while (iterator != NULL && iterator != tree->root && iterator != tree->nil) {
        iterator = iterator->parent;
        ++depth;
    }
    return depth;
}

static size_t tree_count_recursive(const tree_t* tree, const tree_node_t* node) {
    if (node == NULL || node == tree->nil) {
        return 0;
    }
    return 1 + tree_count_recursive(tree, node->left) + tree_count_recursive(tree, node->right);
}

size_t tree_node_count(const tree_t* tree) {
    if (tree == NULL) {
        return 0;
    }
    return tree_count_recursive(tree, tree_root_node(tree));
}

static size_t tree_leaf_count_recursive(const tree_t* tree, const tree_node_t* node) {
    if (node == NULL || node == tree->nil) {
        return 0;
    }
    if (node->left == NULL || node->left == tree->nil) {
        if (node->right == NULL || node->right == tree->nil) {
            return 1;
        }
    }
    return tree_leaf_count_recursive(tree, node->left) + tree_leaf_count_recursive(tree, node->right);
}

size_t tree_leaf_count(const tree_t* tree) {
    if (tree == NULL) {
        return 0;
    }
    return tree_leaf_count_recursive(tree, tree_root_node(tree));
}

static tree_status_t tree_traverse_with_mode(const tree_t* tree,
                                             tree_visit_fn visit,
                                             void* user_ctx,
                                             tree_iterator_mode_t mode) {
    if (tree == NULL || visit == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree_iterator_t iterator;
    tree_status_t status = tree_iterator_init(tree, mode, &iterator);
    if (status != TREE_OK) {
        return status;
    }

    while (tree_iterator_has_next(&iterator)) {
        const void* value = tree_iterator_next(&iterator);
        if (value == NULL) {
            break;
        }
        if (visit(value, user_ctx) != 0) {
            break;
        }
    }
    tree_iterator_destroy(&iterator);
    return TREE_OK;
}

tree_status_t tree_traverse_preorder(const tree_t* tree, tree_visit_fn visit, void* user_ctx) {
    return tree_traverse_with_mode(tree, visit, user_ctx, TREE_ITERATOR_PREORDER);
}

tree_status_t tree_traverse_inorder(const tree_t* tree, tree_visit_fn visit, void* user_ctx) {
    return tree_traverse_with_mode(tree, visit, user_ctx, TREE_ITERATOR_INORDER);
}

tree_status_t tree_traverse_postorder(const tree_t* tree, tree_visit_fn visit, void* user_ctx) {
    return tree_traverse_with_mode(tree, visit, user_ctx, TREE_ITERATOR_POSTORDER);
}

tree_status_t tree_traverse_level_order(const tree_t* tree, tree_visit_fn visit, void* user_ctx) {
    return tree_traverse_with_mode(tree, visit, user_ctx, TREE_ITERATOR_LEVEL_ORDER);
}

/* Generic dispatch wrappers for ordered tree operations */

tree_status_t tree_insert(tree_t* tree, const void* value) {
    if (tree == NULL || value == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    switch (tree->type) {
        case TREE_TYPE_BST:
            return bst_insert(tree, value);
        case TREE_TYPE_AVL:
            return avl_insert(tree, value);
        case TREE_TYPE_RB:
            return rb_insert(tree, value);
        default:
            return TREE_ERR_UNSUPPORTED;
    }
}

tree_status_t tree_remove(tree_t* tree, const void* key) {
    if (tree == NULL || key == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    switch (tree->type) {
        case TREE_TYPE_BST:
            return bst_remove(tree, key);
        case TREE_TYPE_AVL:
            return avl_remove(tree, key);
        case TREE_TYPE_RB:
            return rb_remove(tree, key);
        default:
            return TREE_ERR_UNSUPPORTED;
    }
}

const void* tree_search(const tree_t* tree, const void* key) {
    if (tree == NULL || key == NULL) {
        return NULL;
    }
    switch (tree->type) {
        case TREE_TYPE_BST:
        case TREE_TYPE_AVL:
            return bst_search(tree, key);
        case TREE_TYPE_RB:
            return rb_search(tree, key);
        default:
            return NULL;
    }
}

bool tree_contains(const tree_t* tree, const void* key) {
    return tree_search(tree, key) != NULL;
}

#ifdef __cplusplus
}
#endif
