#ifndef TREE_SERIALIZER_H
#define TREE_SERIALIZER_H

#include <stdio.h>
#include "tree_base.h"

#ifdef __cplusplus
extern "C" {
#endif

tree_status_t tree_serialize_binary(const tree_t* tree, FILE* out);
tree_status_t tree_deserialize_binary(tree_t* tree, FILE* in);
tree_status_t tree_export_json(const tree_t* tree, FILE* out);

#ifdef __cplusplus
}
#endif

#endif /* TREE_SERIALIZER_H */
