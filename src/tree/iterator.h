#ifndef TREE_ITERATOR_H
#define TREE_ITERATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "tree_base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum tree_iterator_mode {
    TREE_ITERATOR_INORDER = 0,
    TREE_ITERATOR_PREORDER,
    TREE_ITERATOR_POSTORDER,
    TREE_ITERATOR_LEVEL_ORDER,
} tree_iterator_mode_t;

struct tree_iterator {
    const tree_t* tree;
    tree_iterator_mode_t mode;
    tree_node_t** nodes;
    size_t count;
    size_t position;
};

tree_status_t tree_iterator_init(const tree_t* tree, tree_iterator_mode_t mode, tree_iterator_t* iterator);
const void* tree_iterator_next(tree_iterator_t* iterator);
const void* tree_iterator_prev(tree_iterator_t* iterator);
bool tree_iterator_has_next(const tree_iterator_t* iterator);
bool tree_iterator_has_prev(const tree_iterator_t* iterator);
void tree_iterator_destroy(tree_iterator_t* iterator);

#ifdef __cplusplus
}
#endif

#endif /* TREE_ITERATOR_H */
