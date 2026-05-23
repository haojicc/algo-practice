#ifndef AVL_TREE_H
#define AVL_TREE_H

#include "tree_base.h"

#ifdef __cplusplus
extern "C" {
#endif

tree_status_t avl_init(tree_t* tree, const tree_config_t* config, size_t element_size);
tree_status_t avl_insert(tree_t* tree, const void* value);
tree_status_t avl_remove(tree_t* tree, const void* key);
const void* avl_search(const tree_t* tree, const void* key);
tree_status_t avl_validate(const tree_t* tree);

#ifdef __cplusplus
}
#endif

#endif /* AVL_TREE_H */
