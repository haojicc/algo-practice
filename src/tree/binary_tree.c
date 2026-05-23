#include "binary_tree.h"
#include <stdlib.h>
#include <string.h>

static tree_node_t* bt_create_node(tree_t* tree, const void* value) {
    if (tree == NULL || value == NULL) {
        return NULL;
    }

    size_t size = sizeof(tree_node_t) + tree->element_size;
    tree_node_t* node = tree->allocator.malloc_fn(size, tree->allocator.ctx);
    if (node == NULL) {
        return NULL;
    }

    node->left = node->right = node->parent = NULL;
    node->height = 1;
    node->color = RB_BLACK;
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

static int bt_node_depth(const tree_t* tree, const tree_node_t* node) {
    if (tree == NULL || node == NULL) {
        return -1;
    }
    int depth = 0;
    const tree_node_t* iterator = node;
    while (iterator != NULL && iterator != tree->root) {
        iterator = iterator->parent;
        ++depth;
    }
    return depth;
}

static size_t bt_height_recursive(const tree_node_t* node) {
    if (node == NULL) {
        return 0;
    }
    size_t left_height = bt_height_recursive(node->left);
    size_t right_height = bt_height_recursive(node->right);
    return 1 + (left_height > right_height ? left_height : right_height);
}

static void bt_remove_recursive(tree_t* tree, tree_node_t* node) {
    if (node == NULL) {
        return;
    }
    bt_remove_recursive(tree, node->left);
    bt_remove_recursive(tree, node->right);
    if (tree->config.destroy) {
        tree->config.destroy(node->payload, tree->config.user_ctx);
    }
    tree->allocator.free_fn(node, tree->allocator.ctx);
}

tree_status_t bt_init(tree_t* tree, const tree_config_t* config, size_t element_size) {
    return tree_init(tree, TREE_TYPE_BINARY, config, element_size);
}

tree_status_t bt_insert_left(tree_t* tree, tree_node_t* parent, const void* value) {
    if (tree == NULL || value == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree_node_t* node = bt_create_node(tree, value);
    if (node == NULL) {
        return TREE_ERR_OOM;
    }

    if (parent == NULL) {
        if (tree->root != NULL) {
            tree->allocator.free_fn(node, tree->allocator.ctx);
            return TREE_ERR_DUPLICATE;
        }
        tree->root = node;
        tree->count = 1;
        return TREE_OK;
    }

    if (parent->left != NULL) {
        tree->allocator.free_fn(node, tree->allocator.ctx);
        return TREE_ERR_DUPLICATE;
    }

    parent->left = node;
    node->parent = parent;
    tree->count += 1;
    return TREE_OK;
}

tree_status_t bt_insert_right(tree_t* tree, tree_node_t* parent, const void* value) {
    if (tree == NULL || value == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree_node_t* node = bt_create_node(tree, value);
    if (node == NULL) {
        return TREE_ERR_OOM;
    }

    if (parent == NULL) {
        if (tree->root != NULL) {
            tree->allocator.free_fn(node, tree->allocator.ctx);
            return TREE_ERR_DUPLICATE;
        }
        tree->root = node;
        tree->count = 1;
        return TREE_OK;
    }

    if (parent->right != NULL) {
        tree->allocator.free_fn(node, tree->allocator.ctx);
        return TREE_ERR_DUPLICATE;
    }

    parent->right = node;
    node->parent = parent;
    tree->count += 1;
    return TREE_OK;
}

tree_status_t bt_remove(tree_t* tree, tree_node_t* node) {
    if (tree == NULL || node == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    if (node != tree->root && node->parent == NULL) {
        return TREE_ERR_NOT_FOUND;
    }

    if (node == tree->root) {
        bt_remove_recursive(tree, tree->root);
        tree->root = NULL;
        tree->count = 0;
        return TREE_OK;
    }

    if (node->parent->left == node) {
        node->parent->left = NULL;
    } else if (node->parent->right == node) {
        node->parent->right = NULL;
    }
    node->parent = NULL;
    bt_remove_recursive(tree, node);
    tree->count -= 1;
    return TREE_OK;
}

tree_status_t bt_remove_subtree(tree_t* tree, tree_node_t* node) {
    if (tree == NULL || node == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    if (node == tree->root) {
        bt_remove_recursive(tree, tree->root);
        tree->root = NULL;
        tree->count = 0;
        return TREE_OK;
    }
    if (node->parent == NULL) {
        return TREE_ERR_NOT_FOUND;
    }
    if (node->parent->left == node) {
        node->parent->left = NULL;
    } else if (node->parent->right == node) {
        node->parent->right = NULL;
    }
    node->parent = NULL;
    bt_remove_recursive(tree, (tree_node_t*)node);
    tree->count = tree_node_count(tree);
    return TREE_OK;
}

int bt_height(tree_t* tree) {
    if (tree == NULL) {
        return 0;
    }
    return (int)bt_height_recursive(tree->root);
}

size_t bt_depth(tree_t* tree, const tree_node_t* node) {
    return (size_t)bt_node_depth(tree, node);
}

size_t bt_node_count(tree_t* tree) {
    return tree_node_count(tree);
}

size_t bt_leaf_count(tree_t* tree) {
    return tree_leaf_count(tree);
}
