#include "iterator.h"
#include <stdlib.h>
#include <string.h>

static tree_status_t tree_iterator_collect_nodes(const tree_t* tree,
                                                tree_iterator_mode_t mode,
                                                tree_node_t*** out_nodes,
                                                size_t* out_count) {
    if (tree == NULL || out_nodes == NULL || out_count == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    size_t capacity = 64;
    tree_node_t** nodes = (tree_node_t**)malloc(capacity * sizeof(tree_node_t*));
    if (nodes == NULL) {
        return TREE_ERR_OOM;
    }
    *out_count = 0;

    if (tree->root == NULL || tree->root == tree->nil) {
        *out_nodes = nodes;
        return TREE_OK;
    }

    if (mode == TREE_ITERATOR_LEVEL_ORDER) {
        size_t queue_capacity = 64;
        tree_node_t** queue = (tree_node_t**)malloc(queue_capacity * sizeof(tree_node_t*));
        if (queue == NULL) {
            free(nodes);
            return TREE_ERR_OOM;
        }
        size_t head = 0;
        size_t tail = 0;
        queue[tail++] = tree->root;

        while (head < tail) {
            if (tail >= queue_capacity) {
                queue_capacity *= 2;
                tree_node_t** next_queue = (tree_node_t**)realloc(queue, queue_capacity * sizeof(tree_node_t*));
                if (next_queue == NULL) {
                    free(nodes);
                    free(queue);
                    return TREE_ERR_OOM;
                }
                queue = next_queue;
            }
            tree_node_t* node = queue[head++];
            if (*out_count >= capacity) {
                capacity *= 2;
                tree_node_t** next_nodes = (tree_node_t**)realloc(nodes, capacity * sizeof(tree_node_t*));
                if (next_nodes == NULL) {
                    free(nodes);
                    free(queue);
                    return TREE_ERR_OOM;
                }
                nodes = next_nodes;
            }
            nodes[(*out_count)++] = node;
            if (node->left && node->left != tree->nil) {
                queue[tail++] = node->left;
            }
            if (node->right && node->right != tree->nil) {
                queue[tail++] = node->right;
            }
        }

        free(queue);
        *out_nodes = nodes;
        return TREE_OK;
    }

    typedef struct stack_item {
        tree_node_t* node;
        int stage;
    } stack_item_t;

    size_t stack_capacity = 64;
    stack_item_t* stack = (stack_item_t*)malloc(stack_capacity * sizeof(stack_item_t));
    if (stack == NULL) {
        free(nodes);
        return TREE_ERR_OOM;
    }
    size_t stack_size = 0;
    stack[stack_size++] = (stack_item_t){tree->root, 0};

    while (stack_size > 0) {
        stack_item_t item = stack[--stack_size];
        tree_node_t* node = item.node;
        if (node == NULL || node == tree->nil) {
            continue;
        }

        if (mode == TREE_ITERATOR_PREORDER) {
            if (*out_count >= capacity) {
                capacity *= 2;
                tree_node_t** next_nodes = (tree_node_t**)realloc(nodes, capacity * sizeof(tree_node_t*));
                if (next_nodes == NULL) {
                    free(nodes);
                    free(stack);
                    return TREE_ERR_OOM;
                }
                nodes = next_nodes;
            }
            nodes[(*out_count)++] = node;
            if (node->right && node->right != tree->nil) {
                if (stack_size >= stack_capacity) {
                    stack_capacity *= 2;
                    stack_item_t* next_stack = (stack_item_t*)realloc(stack, stack_capacity * sizeof(stack_item_t));
                    if (next_stack == NULL) {
                        free(nodes);
                        free(stack);
                        return TREE_ERR_OOM;
                    }
                    stack = next_stack;
                }
                stack[stack_size++] = (stack_item_t){node->right, 0};
            }
            if (node->left && node->left != tree->nil) {
                if (stack_size >= stack_capacity) {
                    stack_capacity *= 2;
                    stack_item_t* next_stack = (stack_item_t*)realloc(stack, stack_capacity * sizeof(stack_item_t));
                    if (next_stack == NULL) {
                        free(nodes);
                        free(stack);
                        return TREE_ERR_OOM;
                    }
                    stack = next_stack;
                }
                stack[stack_size++] = (stack_item_t){node->left, 0};
            }
        } else if (mode == TREE_ITERATOR_POSTORDER) {
            if (item.stage == 0) {
                if (stack_size + 3 >= stack_capacity) {
                    stack_capacity *= 2;
                    stack_item_t* next_stack = (stack_item_t*)realloc(stack, stack_capacity * sizeof(stack_item_t));
                    if (next_stack == NULL) {
                        free(nodes);
                        free(stack);
                        return TREE_ERR_OOM;
                    }
                    stack = next_stack;
                }
                stack[stack_size++] = (stack_item_t){node, 1};
                if (node->right && node->right != tree->nil) {
                    stack[stack_size++] = (stack_item_t){node->right, 0};
                }
                if (node->left && node->left != tree->nil) {
                    stack[stack_size++] = (stack_item_t){node->left, 0};
                }
            } else {
                if (*out_count >= capacity) {
                    capacity *= 2;
                    tree_node_t** next_nodes = (tree_node_t**)realloc(nodes, capacity * sizeof(tree_node_t*));
                    if (next_nodes == NULL) {
                        free(nodes);
                        free(stack);
                        return TREE_ERR_OOM;
                    }
                    nodes = next_nodes;
                }
                nodes[(*out_count)++] = node;
            }
        } else {
            if (item.stage == 0) {
                if (stack_size + 3 >= stack_capacity) {
                    stack_capacity *= 2;
                    stack_item_t* next_stack = (stack_item_t*)realloc(stack, stack_capacity * sizeof(stack_item_t));
                    if (next_stack == NULL) {
                        free(nodes);
                        free(stack);
                        return TREE_ERR_OOM;
                    }
                    stack = next_stack;
                }
                stack[stack_size++] = (stack_item_t){node, 1};
                if (node->right && node->right != tree->nil) {
                    stack[stack_size++] = (stack_item_t){node->right, 0};
                }
                if (node->left && node->left != tree->nil) {
                    stack[stack_size++] = (stack_item_t){node->left, 0};
                }
            } else {
                if (*out_count >= capacity) {
                    capacity *= 2;
                    tree_node_t** next_nodes = (tree_node_t**)realloc(nodes, capacity * sizeof(tree_node_t*));
                    if (next_nodes == NULL) {
                        free(nodes);
                        free(stack);
                        return TREE_ERR_OOM;
                    }
                    nodes = next_nodes;
                }
                nodes[(*out_count)++] = node;
            }
        }
    }
    free(stack);
    *out_nodes = nodes;
    return TREE_OK;
}

static tree_status_t tree_iterator_fill(tree_iterator_t* iterator) {
    if (iterator == NULL || iterator->tree == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    size_t count = 0;
    tree_node_t** nodes = NULL;
    tree_status_t status = tree_iterator_collect_nodes(iterator->tree, iterator->mode, &nodes, &count);
    if (status != TREE_OK) {
        return status;
    }
    iterator->nodes = nodes;
    iterator->count = count;
    iterator->position = 0;
    return TREE_OK;
}

/* Public API */

tree_status_t tree_iterator_init(const tree_t* tree, tree_iterator_mode_t mode, tree_iterator_t* iterator) {
    if (tree == NULL || iterator == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    iterator->tree = tree;
    iterator->mode = mode;
    iterator->nodes = NULL;
    iterator->count = 0;
    iterator->position = 0;
    return tree_iterator_fill(iterator);
}

const void* tree_iterator_next(tree_iterator_t* iterator) {
    if (iterator == NULL || iterator->position >= iterator->count) {
        return NULL;
    }
    return iterator->nodes[iterator->position++]->payload;
}

const void* tree_iterator_prev(tree_iterator_t* iterator) {
    if (iterator == NULL || iterator->position == 0) {
        return NULL;
    }
    iterator->position -= 1;
    return iterator->nodes[iterator->position]->payload;
}

bool tree_iterator_has_next(const tree_iterator_t* iterator) {
    return iterator != NULL && iterator->position < iterator->count;
}

bool tree_iterator_has_prev(const tree_iterator_t* iterator) {
    return iterator != NULL && iterator->position > 0 && iterator->count > 0;
}

void tree_iterator_destroy(tree_iterator_t* iterator) {
    if (iterator == NULL) {
        return;
    }
    free(iterator->nodes);
    iterator->nodes = NULL;
    iterator->count = 0;
    iterator->position = 0;
}
