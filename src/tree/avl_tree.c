#include "avl_tree.h"
#include <stdlib.h>
#include <string.h>

static int avl_compare(const tree_t* tree, const void* lhs, const void* rhs) {
    return tree->config.compare(lhs, rhs, tree->config.user_ctx);
}

static int avl_node_height(const tree_node_t* node) {
    return node ? node->height : 0;
}

static void avl_update_height(tree_node_t* node) {
    if (node == NULL) {
        return;
    }
    int left_height = avl_node_height(node->left);
    int right_height = avl_node_height(node->right);
    node->height = (left_height > right_height ? left_height : right_height) + 1;
}

static int avl_balance_factor(const tree_node_t* node) {
    if (node == NULL) {
        return 0;
    }
    return avl_node_height(node->left) - avl_node_height(node->right);
}

static void avl_set_parent(tree_node_t* child, tree_node_t* parent) {
    if (child != NULL) {
        child->parent = parent;
    }
}

static tree_node_t* avl_rotate_right(tree_t* tree, tree_node_t* y) {
    tree_node_t* x = y->left;
    tree_node_t* t2 = x->right;

    x->right = y;
    y->left = t2;

    x->parent = y->parent;
    y->parent = x;
    avl_set_parent(t2, y);

    avl_update_height(y);
    avl_update_height(x);

    if (x->parent == NULL) {
        tree->root = x;
    } else if (x->parent->left == y) {
        x->parent->left = x;
    } else {
        x->parent->right = x;
    }
    return x;
}

static tree_node_t* avl_rotate_left(tree_t* tree, tree_node_t* x) {
    tree_node_t* y = x->right;
    tree_node_t* t2 = y->left;

    y->left = x;
    x->right = t2;

    y->parent = x->parent;
    x->parent = y;
    avl_set_parent(t2, x);

    avl_update_height(x);
    avl_update_height(y);

    if (y->parent == NULL) {
        tree->root = y;
    } else if (y->parent->left == x) {
        y->parent->left = y;
    } else {
        y->parent->right = y;
    }
    return y;
}

static tree_node_t* avl_rebalance(tree_t* tree, tree_node_t* node) {
    avl_update_height(node);
    int balance = avl_balance_factor(node);

    if (balance > 1 && avl_balance_factor(node->left) >= 0) {
        return avl_rotate_right(tree, node);
    }
    if (balance > 1 && avl_balance_factor(node->left) < 0) {
        node->left = avl_rotate_left(tree, node->left);
        return avl_rotate_right(tree, node);
    }
    if (balance < -1 && avl_balance_factor(node->right) <= 0) {
        return avl_rotate_left(tree, node);
    }
    if (balance < -1 && avl_balance_factor(node->right) > 0) {
        node->right = avl_rotate_right(tree, node->right);
        return avl_rotate_left(tree, node);
    }
    return node;
}

static tree_node_t* avl_find_node(const tree_t* tree, const void* key) {
    tree_node_t* node = tree->root;
    while (node != NULL) {
        int cmp = avl_compare(tree, key, node->payload);
        if (cmp == 0) {
            return node;
        }
        node = (cmp < 0) ? node->left : node->right;
    }
    return NULL;
}

static tree_node_t* avl_min_node(tree_node_t* node) {
    while (node != NULL && node->left != NULL) {
        node = node->left;
    }
    return node;
}

static tree_status_t avl_rebalance_path(tree_t* tree, tree_node_t* node) {
    while (node != NULL) {
        tree_node_t* parent = node->parent;
        avl_rebalance(tree, node);
        node = parent;
    }
    return TREE_OK;
}

static tree_status_t avl_remove_internal(tree_t* tree, tree_node_t* node) {
    tree_node_t* parent = node->parent;
    tree_node_t* replacement = NULL;

    if (node->left == NULL) {
        replacement = node->right;
        if (replacement) {
            replacement->parent = parent;
        }
        if (parent == NULL) {
            tree->root = replacement;
        } else if (parent->left == node) {
            parent->left = replacement;
        } else {
            parent->right = replacement;
        }
    } else if (node->right == NULL) {
        replacement = node->left;
        replacement->parent = parent;
        if (parent == NULL) {
            tree->root = replacement;
        } else if (parent->left == node) {
            parent->left = replacement;
        } else {
            parent->right = replacement;
        }
    } else {
        tree_node_t* successor = avl_min_node(node->right);
        if (successor->parent != node) {
            successor->parent->left = successor->right;
            if (successor->right) {
                successor->right->parent = successor->parent;
            }
            successor->right = node->right;
            successor->right->parent = successor;
        }
        successor->left = node->left;
        successor->left->parent = successor;
        successor->parent = node->parent;

        if (node->parent == NULL) {
            tree->root = successor;
        } else if (node->parent->left == node) {
            node->parent->left = successor;
        } else {
            node->parent->right = successor;
        }
        replacement = successor->parent;

        if (tree->config.destroy) {
            tree->config.destroy(node->payload, tree->config.user_ctx);
        }
        tree->config.copy(successor->payload, node->payload, tree->element_size, tree->config.user_ctx);
        node = successor;
    }

    tree_node_destroy(tree, node);
    tree->count -= 1;
    return avl_rebalance_path(tree, replacement);
}

/* Public API */

tree_status_t avl_init(tree_t* tree, const tree_config_t* config, size_t element_size) {
    return tree_init(tree, TREE_TYPE_AVL, config, element_size);
}

tree_status_t avl_insert(tree_t* tree, const void* value) {
    if (tree == NULL || value == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree_node_t* parent = NULL;
    tree_node_t* current = tree->root;
    int cmp = 0;

    while (current != NULL) {
        parent = current;
        cmp = avl_compare(tree, value, current->payload);
        if (cmp == 0) {
            return TREE_ERR_DUPLICATE;
        }
        current = (cmp < 0) ? current->left : current->right;
    }

    tree_node_t* node = tree_node_create(tree, value);
    if (node == NULL) {
        return TREE_ERR_OOM;
    }
    node->parent = parent;
    if (parent == NULL) {
        tree->root = node;
    } else if (cmp < 0) {
        parent->left = node;
    } else {
        parent->right = node;
    }

    tree->count += 1;
    return avl_rebalance_path(tree, node);
}

tree_status_t avl_remove(tree_t* tree, const void* key) {
    if (tree == NULL || key == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    tree_node_t* node = avl_find_node(tree, key);
    if (node == NULL) {
        return TREE_ERR_NOT_FOUND;
    }
    return avl_remove_internal(tree, node);
}

const void* avl_search(const tree_t* tree, const void* key) {
    tree_node_t* node = avl_find_node(tree, key);
    return node ? node->payload : NULL;
}

tree_status_t avl_validate(const tree_t* tree) {
    if (tree == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    size_t visited = 0;
    tree_node_t** stack = NULL;
    size_t size = 0;
    size_t capacity = 0;
    const void* previous = NULL;
    tree_node_t* node = tree->root;

    while (node != NULL || size > 0) {
        while (node != NULL) {
            if (size >= capacity) {
                size_t next = capacity == 0 ? 64 : capacity * 2;
                tree_node_t** next_stack = (tree_node_t**)realloc(stack, next * sizeof(tree_node_t*));
                if (next_stack == NULL) {
                    free(stack);
                    return TREE_ERR_OOM;
                }
                stack = next_stack;
                capacity = next;
            }
            stack[size++] = node;
            node = node->left;
        }

        node = stack[--size];
        if (previous != NULL && avl_compare(tree, previous, node->payload) >= 0) {
            free(stack);
            return TREE_ERR_CORRUPTED;
        }

        int left_height = avl_node_height(node->left);
        int right_height = avl_node_height(node->right);
        int expected_height = (left_height > right_height ? left_height : right_height) + 1;
        if (node->height != expected_height) {
            free(stack);
            return TREE_ERR_CORRUPTED;
        }
        int factor = left_height - right_height;
        if (factor < -1 || factor > 1) {
            free(stack);
            return TREE_ERR_CORRUPTED;
        }

        previous = node->payload;
        node = node->right;
        ++visited;
    }

    if (visited != tree->count) {
        free(stack);
        return TREE_ERR_CORRUPTED;
    }

    free(stack);
    return TREE_OK;
}
