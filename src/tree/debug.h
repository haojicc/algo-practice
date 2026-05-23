#ifndef TREE_DEBUG_H
#define TREE_DEBUG_H

#include <stdio.h>
#include "tree_base.h"

#ifdef __cplusplus
extern "C" {
#endif

tree_status_t tree_validate(const tree_t* tree);
tree_status_t tree_dump_dot(const tree_t* tree, FILE* out);
tree_status_t tree_dump_text(const tree_t* tree, FILE* out);

#ifdef __cplusplus
}
#endif

#endif /* TREE_DEBUG_H */
