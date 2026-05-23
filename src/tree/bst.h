#ifndef BST_H
#define BST_H

#include "tree_base.h"

#ifdef __cplusplus
extern "C" {
#endif

tree_status_t bst_init(tree_t* tree, const tree_config_t* config, size_t element_size);
tree_status_t bst_insert(tree_t* tree, const void* value);
tree_status_t bst_remove(tree_t* tree, const void* key);
const void* bst_search(const tree_t* tree, const void* key);
bool bst_contains(const tree_t* tree, const void* key);
const void* bst_min(const tree_t* tree);
const void* bst_max(const tree_t* tree);
const void* bst_predecessor(const tree_t* tree, const void* key);
const void* bst_successor(const tree_t* tree, const void* key);
tree_status_t bst_lower_bound(const tree_t* tree, const void* key, const void** out_value);
tree_status_t bst_upper_bound(const tree_t* tree, const void* key, const void** out_value);
tree_status_t bst_range_query(const tree_t* tree,
                              const void* start_key,
                              const void* end_key,
                              tree_visit_fn visit,
                              void* user_ctx);

#ifdef __cplusplus
}
#endif

#endif /* BST_H */
