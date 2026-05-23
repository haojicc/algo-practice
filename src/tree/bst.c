#include "bst.h"
#include <stdlib.h>
#include <string.h>

static int bst_compare(const tree_t* tree, const void* lhs, const void* rhs) {
    return tree->config.compare(lhs, rhs, tree->config.user_ctx);
}

static tree_node_t* bst_find_node(const tree_t* tree, const void* key) {
    tree_node_t* current = tree->root;
    while (current != NULL) {
        int cmp = bst_compare(tree, key, current->payload);
        if (cmp == 0) {
            return current;
        }
        current = (cmp < 0) ? current->left : current->right;
    }
    return NULL;
}

static tree_node_t* bst_min_node(const tree_t* tree, tree_node_t* node) {
    while (node != NULL && node->left != NULL) {
        node = node->left;
    }
    return node;
}

static tree_node_t* bst_max_node(const tree_t* tree, tree_node_t* node) {
    while (node != NULL && node->right != NULL) {
        node = node->right;
    }
    return node;
}

static tree_node_t* bst_successor_node(tree_node_t* node) {
    if (node == NULL) {
        return NULL;
    }
    if (node->right != NULL) {
        node = node->right;
        while (node->left != NULL) {
            node = node->left;
        }
        return node;
    }
    tree_node_t* parent = node->parent;
    while (parent != NULL && node == parent->right) {
        node = parent;
        parent = parent->parent;
    }
    return parent;
}

static tree_node_t* bst_predecessor_node(tree_node_t* node) {
    if (node == NULL) {
        return NULL;
    }
    if (node->left != NULL) {
        node = node->left;
        while (node->right != NULL) {
            node = node->right;
        }
        return node;
    }
    tree_node_t* parent = node->parent;
    while (parent != NULL && node == parent->left) {
        node = parent;
        parent = parent->parent;
    }
    return parent;
}

static void bst_transplant(tree_t* tree, tree_node_t* u, tree_node_t* v) {
    if (u->parent == NULL) {
        tree->root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v != NULL) {
        v->parent = u->parent;
    }
}

static tree_status_t bst_remove_node(tree_t* tree, tree_node_t* node) {
    if (node->left == NULL) {
        bst_transplant(tree, node, node->right);
    } else if (node->right == NULL) {
        bst_transplant(tree, node, node->left);
    } else {
        tree_node_t* successor = bst_min_node(tree, node->right);
        if (successor->parent != node) {
            bst_transplant(tree, successor, successor->right);
            successor->right = node->right;
            if (successor->right) {
                successor->right->parent = successor;
            }
        }
        bst_transplant(tree, node, successor);
        successor->left = node->left;
        if (successor->left) {
            successor->left->parent = successor;
        }
    }

    tree_node_destroy(tree, node);
    tree->count -= 1;
    return TREE_OK;
}

static tree_status_t bst_replace_with_successor(tree_t* tree, tree_node_t* target) {
    tree_node_t* successor = bst_min_node(tree, target->right);
    if (successor == NULL) {
        return TREE_ERR_NOT_FOUND;
    }

    if (tree->config.destroy) {
        tree->config.destroy(target->payload, tree->config.user_ctx);
    }
    tree->config.copy(successor->payload, target->payload, tree->element_size, tree->config.user_ctx);
    return bst_remove_node(tree, successor);
}

static void bst_walk_range(tree_node_t* node,
                           const tree_t* tree,
                           const void* start_key,
                           const void* end_key,
                           tree_visit_fn visit,
                           void* user_ctx) {
    if (node == NULL) {
        return;
    }
    int cmp_low = bst_compare(tree, node->payload, start_key);
    int cmp_high = bst_compare(tree, node->payload, end_key);
    if (cmp_low > 0) {
        bst_walk_range(node->left, tree, start_key, end_key, visit, user_ctx);
    }
    if (cmp_low >= 0 && cmp_high <= 0) {
        visit(node->payload, user_ctx);
    }
    if (cmp_high < 0) {
        bst_walk_range(node->right, tree, start_key, end_key, visit, user_ctx);
    }
}

static int visit_stub(const void* value, void* user_ctx) {
    tree_visit_fn visit = (tree_visit_fn)user_ctx;
    return visit(value, NULL);
}


/* Public API */

tree_status_t bst_init(tree_t* tree, const tree_config_t* config, size_t element_size) {
    return tree_init(tree, TREE_TYPE_BST, config, element_size);
}

tree_status_t bst_insert(tree_t* tree, const void* value) {
    if (tree == NULL || value == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    tree_node_t* parent = NULL;
    tree_node_t* cursor = tree->root;
    int cmp = 0;

    while (cursor != NULL) {
        parent = cursor;
        cmp = bst_compare(tree, value, cursor->payload);
        if (cmp == 0) {
            return TREE_ERR_DUPLICATE;
        }
        cursor = (cmp < 0) ? cursor->left : cursor->right;
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
    return TREE_OK;
}

tree_status_t bst_remove(tree_t* tree, const void* key) {
    if (tree == NULL || key == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    tree_node_t* node = bst_find_node(tree, key);
    if (node == NULL) {
        return TREE_ERR_NOT_FOUND;
    }
    if (node->left != NULL && node->right != NULL) {
        return bst_replace_with_successor(tree, node);
    }
    return bst_remove_node(tree, node);
}

const void* bst_search(const tree_t* tree, const void* key) {
    if (tree == NULL || key == NULL) {
        return NULL;
    }
    tree_node_t* node = bst_find_node(tree, key);
    return node ? node->payload : NULL;
}

bool bst_contains(const tree_t* tree, const void* key) {
    return bst_search(tree, key) != NULL;
}

const void* bst_min(const tree_t* tree) {
    if (tree == NULL || tree->root == NULL) {
        return NULL;
    }
    tree_node_t* node = bst_min_node(tree, tree->root);
    return node ? node->payload : NULL;
}

const void* bst_max(const tree_t* tree) {
    if (tree == NULL || tree->root == NULL) {
        return NULL;
    }
    tree_node_t* node = bst_max_node(tree, tree->root);
    return node ? node->payload : NULL;
}

const void* bst_successor(const tree_t* tree, const void* key) {
    if (tree == NULL || key == NULL) {
        return NULL;
    }
    tree_node_t* node = bst_find_node(tree, key);
    if (node == NULL) {
        return NULL;
    }
    tree_node_t* succ = bst_successor_node(node);
    return succ ? succ->payload : NULL;
}

const void* bst_predecessor(const tree_t* tree, const void* key) {
    if (tree == NULL || key == NULL) {
        return NULL;
    }
    tree_node_t* node = bst_find_node(tree, key);
    if (node == NULL) {
        return NULL;
    }
    tree_node_t* pred = bst_predecessor_node(node);
    return pred ? pred->payload : NULL;
}

tree_status_t bst_lower_bound(const tree_t* tree, const void* key, const void** out_value) {
    if (tree == NULL || key == NULL || out_value == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    tree_node_t* best = NULL;
    tree_node_t* cursor = tree->root;
    while (cursor != NULL) {
        int cmp = bst_compare(tree, cursor->payload, key);
        if (cmp >= 0) {
            best = cursor;
            cursor = cursor->left;
        } else {
            cursor = cursor->right;
        }
    }
    *out_value = best ? best->payload : NULL;
    return TREE_OK;
}

tree_status_t bst_upper_bound(const tree_t* tree, const void* key, const void** out_value) {
    if (tree == NULL || key == NULL || out_value == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    tree_node_t* best = NULL;
    tree_node_t* cursor = tree->root;
    while (cursor != NULL) {
        int cmp = bst_compare(tree, cursor->payload, key);
        if (cmp > 0) {
            best = cursor;
            cursor = cursor->left;
        } else {
            cursor = cursor->right;
        }
    }
    *out_value = best ? best->payload : NULL;
    return TREE_OK;
}

tree_status_t bst_range_query(const tree_t* tree,
                              const void* start_key,
                              const void* end_key,
                              tree_visit_fn visit,
                              void* user_ctx) {
    if (tree == NULL || start_key == NULL || end_key == NULL || visit == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    bst_walk_range(tree->root, tree, start_key, end_key, visit, user_ctx);
    return TREE_OK;
}
