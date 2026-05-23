#ifndef BINARY_TREE_H
#define BINARY_TREE_H

#include "tree_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Binary tree API */

tree_status_t bt_init(tree_t* tree, const tree_config_t* config, size_t element_size);
tree_status_t bt_insert_left(tree_t* tree, tree_node_t* parent, const void* value);
tree_status_t bt_insert_right(tree_t* tree, tree_node_t* parent, const void* value);
tree_status_t bt_remove(tree_t* tree, tree_node_t* node);
tree_status_t bt_remove_subtree(tree_t* tree, tree_node_t* node);
int bt_height(tree_t* tree);
size_t bt_depth(tree_t* tree, const tree_node_t* node);
size_t bt_node_count(tree_t* tree);
size_t bt_leaf_count(tree_t* tree);

#ifdef __cplusplus
}
#endif

#endif /* BINARY_TREE_H */
