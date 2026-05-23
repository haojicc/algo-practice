#include "rb_tree.h"
#include <stdlib.h>
#include <string.h>

static int rb_compare(const tree_t* tree, const void* lhs, const void* rhs) {
    return tree->config.compare(lhs, rhs, tree->config.user_ctx);
}

static void rb_set_child(tree_node_t* parent, tree_node_t* child, bool left) {
    if (left) {
        parent->left = child;
    } else {
        parent->right = child;
    }
    if (child != NULL) {
        child->parent = parent;
    }
}

static void rb_rotate_left(tree_t* tree, tree_node_t* x) {
    tree_node_t* y = x->right;
    x->right = y->left;
    if (y->left != tree->nil) {
        y->left->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == tree->nil) {
        tree->root = y;
    } else if (x == x->parent->left) {
        x->parent->left = y;
    } else {
        x->parent->right = y;
    }
    y->left = x;
    x->parent = y;
}

static void rb_rotate_right(tree_t* tree, tree_node_t* x) {
    tree_node_t* y = x->left;
    x->left = y->right;
    if (y->right != tree->nil) {
        y->right->parent = x;
    }
    y->parent = x->parent;
    if (x->parent == tree->nil) {
        tree->root = y;
    } else if (x == x->parent->right) {
        x->parent->right = y;
    } else {
        x->parent->left = y;
    }
    y->right = x;
    x->parent = y;
}

static void rb_insert_fixup(tree_t* tree, tree_node_t* node) {
    while (node->parent->color == RB_RED) {
        if (node->parent == node->parent->parent->left) {
            tree_node_t* uncle = node->parent->parent->right;
            if (uncle->color == RB_RED) {
                node->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                node = node->parent->parent;
            } else {
                if (node == node->parent->right) {
                    node = node->parent;
                    rb_rotate_left(tree, node);
                }
                node->parent->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                rb_rotate_right(tree, node->parent->parent);
            }
        } else {
            tree_node_t* uncle = node->parent->parent->left;
            if (uncle->color == RB_RED) {
                node->parent->color = RB_BLACK;
                uncle->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                node = node->parent->parent;
            } else {
                if (node == node->parent->left) {
                    node = node->parent;
                    rb_rotate_right(tree, node);
                }
                node->parent->color = RB_BLACK;
                node->parent->parent->color = RB_RED;
                rb_rotate_left(tree, node->parent->parent);
            }
        }
    }
    tree->root->color = RB_BLACK;
}

static tree_node_t* rb_find_node(const tree_t* tree, const void* key) {
    tree_node_t* current = tree->root;
    while (current != tree->nil) {
        int cmp = rb_compare(tree, key, current->payload);
        if (cmp == 0) {
            return current;
        }
        current = (cmp < 0) ? current->left : current->right;
    }
    return NULL;
}

static tree_node_t* rb_min_node(const tree_t* tree, tree_node_t* node) {
    while (node != tree->nil) {
        if (node->left == tree->nil) {
            break;
        }
        node = node->left;
    }
    return node;
}

static void rb_transplant(tree_t* tree, tree_node_t* u, tree_node_t* v) {
    if (u->parent == tree->nil) {
        tree->root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    v->parent = u->parent;
}

static void rb_delete_fixup(tree_t* tree, tree_node_t* x) {
    while (x != tree->root && x->color == RB_BLACK) {
        if (x == x->parent->left) {
            tree_node_t* w = x->parent->right;
            if (w->color == RB_RED) {
                w->color = RB_BLACK;
                x->parent->color = RB_RED;
                rb_rotate_left(tree, x->parent);
                w = x->parent->right;
            }
            if (w->left->color == RB_BLACK && w->right->color == RB_BLACK) {
                w->color = RB_RED;
                x = x->parent;
            } else {
                if (w->right->color == RB_BLACK) {
                    w->left->color = RB_BLACK;
                    w->color = RB_RED;
                    rb_rotate_right(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK;
                w->right->color = RB_BLACK;
                rb_rotate_left(tree, x->parent);
                x = tree->root;
            }
        } else {
            tree_node_t* w = x->parent->left;
            if (w->color == RB_RED) {
                w->color = RB_BLACK;
                x->parent->color = RB_RED;
                rb_rotate_right(tree, x->parent);
                w = x->parent->left;
            }
            if (w->right->color == RB_BLACK && w->left->color == RB_BLACK) {
                w->color = RB_RED;
                x = x->parent;
            } else {
                if (w->left->color == RB_BLACK) {
                    w->right->color = RB_BLACK;
                    w->color = RB_RED;
                    rb_rotate_left(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = RB_BLACK;
                w->left->color = RB_BLACK;
                rb_rotate_right(tree, x->parent);
                x = tree->root;
            }
        }
    }
    x->color = RB_BLACK;
}

static tree_status_t rb_delete_node(tree_t* tree, tree_node_t* z) {
    tree_node_t* y = z;
    tree_color_t original_color = y->color;
    tree_node_t* x = tree->nil;

    if (z->left == tree->nil) {
        x = z->right;
        rb_transplant(tree, z, z->right);
    } else if (z->right == tree->nil) {
        x = z->left;
        rb_transplant(tree, z, z->left);
    } else {
        y = rb_min_node(tree, z->right);
        original_color = y->color;
        x = y->right;
        if (y->parent == z) {
            x->parent = y;
        } else {
            rb_transplant(tree, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        rb_transplant(tree, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (tree->config.destroy) {
        tree->config.destroy(z->payload, tree->config.user_ctx);
    }
    tree_node_destroy(tree, z);
    tree->count -= 1;
    if (original_color == RB_BLACK) {
        rb_delete_fixup(tree, x);
    }
    return TREE_OK;
}

static tree_status_t rb_check_properties(const tree_t* tree,
                                         const tree_node_t* node,
                                         int black_count,
                                         int* expected_black) {
    if (node == tree->nil) {
        if (*expected_black < 0) {
            *expected_black = black_count;
        } else if (*expected_black != black_count) {
            return TREE_ERR_CORRUPTED;
        }
        return TREE_OK;
    }

    if (node->color == RB_RED && node->parent != tree->nil && node->parent->color == RB_RED) {
        return TREE_ERR_CORRUPTED;
    }
    if (node->color == RB_BLACK) {
        ++black_count;
    }
    int left_cmp = rb_compare(tree, node->left->payload, node->payload);
    int right_cmp = rb_compare(tree, node->payload, node->right->payload);
    if (node->left != tree->nil && left_cmp >= 0) {
        return TREE_ERR_CORRUPTED;
    }
    if (node->right != tree->nil && right_cmp >= 0) {
        return TREE_ERR_CORRUPTED;
    }
    tree_status_t status = rb_check_properties(tree, node->left, black_count, expected_black);
    if (status != TREE_OK) {
        return status;
    }
    return rb_check_properties(tree, node->right, black_count, expected_black);
}

static tree_status_t rb_validate_tree(const tree_t* tree) {
    if (tree == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    if (tree->root == tree->nil) {
        return TREE_OK;
    }
    if (tree->root->color != RB_BLACK) {
        return TREE_ERR_CORRUPTED;
    }
    int expected_black = -1;
    return rb_check_properties(tree, tree->root, 0, &expected_black);
}

/* Public API */

tree_status_t rb_init(tree_t* tree, const tree_config_t* config, size_t element_size) {
    return tree_init(tree, TREE_TYPE_RB, config, element_size);
}

tree_status_t rb_insert(tree_t* tree, const void* value) {
    if (tree == NULL || value == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    tree_node_t* z = tree_node_create(tree, value);
    if (z == NULL) {
        return TREE_ERR_OOM;
    }
    z->left = z->right = tree->nil;
    z->color = RB_RED;

    tree_node_t* y = tree->nil;
    tree_node_t* x = tree->root;
    while (x != tree->nil) {
        y = x;
        int cmp = rb_compare(tree, z->payload, x->payload);
        if (cmp == 0) {
            tree_node_destroy(tree, z);
            return TREE_ERR_DUPLICATE;
        }
        x = (cmp < 0) ? x->left : x->right;
    }
    z->parent = y;
    if (y == tree->nil) {
        tree->root = z;
    } else if (rb_compare(tree, z->payload, y->payload) < 0) {
        y->left = z;
    } else {
        y->right = z;
    }
    tree->count += 1;
    rb_insert_fixup(tree, z);
    return TREE_OK;
}

tree_status_t rb_remove(tree_t* tree, const void* key) {
    if (tree == NULL || key == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    tree_node_t* node = rb_find_node(tree, key);
    if (node == NULL) {
        return TREE_ERR_NOT_FOUND;
    }
    return rb_delete_node(tree, node);
}

const void* rb_search(const tree_t* tree, const void* key) {
    tree_node_t* node = rb_find_node(tree, key);
    return node ? node->payload : NULL;
}

tree_status_t rb_validate(const tree_t* tree) {
    return rb_validate_tree(tree);
}
