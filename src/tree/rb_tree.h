#ifndef RB_TREE_H
#define RB_TREE_H

#include "tree_base.h"

#ifdef __cplusplus
extern "C" {
#endif

tree_status_t rb_init(tree_t* tree, const tree_config_t* config, size_t element_size);
tree_status_t rb_insert(tree_t* tree, const void* value);
tree_status_t rb_remove(tree_t* tree, const void* key);
const void* rb_search(const tree_t* tree, const void* key);
tree_status_t rb_validate(const tree_t* tree);

#ifdef __cplusplus
}
#endif

#endif /* RB_TREE_H */
