#include "serializer.h"
#include <stdlib.h>
#include <string.h>

typedef struct tree_serialize_context {
    FILE* out;
    const tree_t* tree;
    tree_status_t status;
    size_t count;
} tree_serialize_context_t;

static int tree_serialize_visit(const void* value, void* user_ctx) {
    tree_serialize_context_t* context = (tree_serialize_context_t*)user_ctx;
    if (context->status != TREE_OK) {
        return 1;
    }
    uint32_t record_size = 0;
    uint8_t buffer[1024];

    if (context->tree->config.serialize) {
        tree_status_t status = context->tree->config.serialize(value, buffer, sizeof(buffer), context->tree->config.user_ctx);
        if (status != TREE_OK) {
            context->status = status;
            return 1;
        }
        record_size = (uint32_t)strlen((const char*)buffer);
    } else {
        if (context->tree->element_size > sizeof(buffer)) {
            context->status = TREE_ERR_OOM;
            return 1;
        }
        memcpy(buffer, value, context->tree->element_size);
        record_size = (uint32_t)context->tree->element_size;
    }
    if (fwrite(&record_size, sizeof(record_size), 1, context->out) != 1) {
        context->status = TREE_ERR_IO;
        return 1;
    }
    if (fwrite(buffer, 1, record_size, context->out) != record_size) {
        context->status = TREE_ERR_IO;
        return 1;
    }
    return 0;
}

static int tree_export_json_visit(const void* value, void* user_ctx) {
    tree_serialize_context_t* context = (tree_serialize_context_t*)user_ctx;
    if (context->status != TREE_OK) {
        return 1;
    }
    char buffer[256] = {0};
    if (context->tree->config.print) {
        context->tree->config.print(value, context->tree->config.user_ctx, buffer, sizeof(buffer));
    } else {
        const unsigned char* bytes = (const unsigned char*)value;
        size_t count = context->tree->element_size;
        size_t written = 0;
        for (size_t index = 0; index < count && written + 3 < sizeof(buffer); ++index) {
            written += snprintf(buffer + written, sizeof(buffer) - written, "%02x", bytes[index]);
        }
    }

    fprintf(context->out, "%s\"%s\"",
            context->count > 0 ? ", " : "",
            buffer);
    context->count += 1;
    return 0;
}

static int tree_export_json_visit_wrapper(const void* value, void* user_ctx) {
    return tree_export_json_visit(value, user_ctx);
}

/* Public API */

tree_status_t tree_serialize_binary(const tree_t* tree, FILE* out) {
    if (tree == NULL || out == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    uint32_t type = (uint32_t)tree->type;
    uint32_t size = (uint32_t)tree->element_size;
    uint64_t count = tree->count;
    if (fwrite(&type, sizeof(type), 1, out) != 1 || fwrite(&size, sizeof(size), 1, out) != 1 || fwrite(&count, sizeof(count), 1, out) != 1) {
        return TREE_ERR_IO;
    }

    tree_serialize_context_t ctx = {out, tree, TREE_OK, 0};
    tree_traverse_preorder(tree, tree_serialize_visit, &ctx);
    return ctx.status;
}

tree_status_t tree_deserialize_binary(tree_t* tree, FILE* in) {
    if (tree == NULL || in == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    uint32_t type = 0;
    uint32_t size = 0;
    uint64_t count = 0;
    if (fread(&type, sizeof(type), 1, in) != 1 || fread(&size, sizeof(size), 1, in) != 1 || fread(&count, sizeof(count), 1, in) != 1) {
        return TREE_ERR_IO;
    }
    if (type != (uint32_t)tree->type || size != tree->element_size) {
        return TREE_ERR_INVALID_ARGUMENT;
    }

    if (!tree_is_empty(tree)) {
        tree_clear(tree);
    }

    for (uint64_t index = 0; index < count; ++index) {
        uint32_t record_size;
        if (fread(&record_size, sizeof(record_size), 1, in) != 1) {
            return TREE_ERR_IO;
        }
        char* buffer = (char*)malloc(record_size + 1);
        if (buffer == NULL) {
            return TREE_ERR_OOM;
        }
        if (fread(buffer, 1, record_size, in) != record_size) {
            free(buffer);
            return TREE_ERR_IO;
        }
        buffer[record_size] = '\0';

        uint8_t* value = (uint8_t*)malloc(tree->element_size);
        if (value == NULL) {
            free(buffer);
            return TREE_ERR_OOM;
        }
        tree_status_t status;
        if (tree->config.deserialize) {
            status = tree->config.deserialize(buffer, record_size, value, tree->config.user_ctx);
        } else {
            if (record_size != tree->element_size) {
                free(buffer);
                free(value);
                return TREE_ERR_INVALID_ARGUMENT;
            }
            memcpy(value, buffer, tree->element_size);
            status = TREE_OK;
        }
        free(buffer);
        if (status != TREE_OK) {
            free(value);
            return status;
        }
        status = tree_insert((tree_t*)tree, value);
        free(value);
        if (status != TREE_OK) {
            return status;
        }
    }
    return TREE_OK;
}

tree_status_t tree_export_json(const tree_t* tree, FILE* out) {
    if (tree == NULL || out == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    fprintf(out, "{\n  \"type\": \"%s\",\n  \"count\": %zu,\n  \"values\": [",
            tree->type == TREE_TYPE_AVL ? "avl" : tree->type == TREE_TYPE_RB ? "rb" : tree->type == TREE_TYPE_BST ? "bst" : "binary",
            tree->count);

    tree_serialize_context_t ctx = {out, tree, TREE_OK};
    ctx.count = 0;
    tree_traverse_inorder(tree, tree_export_json_visit_wrapper, &ctx);
    fprintf(out, "]\n}\n");
    return ctx.status;
}
