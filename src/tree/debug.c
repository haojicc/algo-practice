#include "debug.h"
#include <stdlib.h>
#include <string.h>
#include "bst.h"
#include "avl_tree.h"
#include "rb_tree.h"

static void tree_print_value_default(const void* value, void* user_ctx, char* buffer, size_t size) {
    (void)user_ctx;
    if (value == NULL || buffer == NULL || size == 0) {
        return;
    }
    const unsigned char* bytes = (const unsigned char*)value;
    size_t written = 0;
    size_t count = size > 2 ? size - 1 : 0;
    for (size_t index = 0; index + 2 < count; ++index) {
        written += snprintf(buffer + written, size - written, "%02x", bytes[index]);
        if (written + 2 >= size) {
            break;
        }
    }
    buffer[size - 1] = '\0';
}

static int tree_dump_text_node(const tree_t* tree,
                               const tree_node_t* node,
                               FILE* out,
                               int depth) {
    if (node == NULL || node == tree->nil) {
        return 0;
    }
    for (int i = 0; i < depth; ++i) {
        fprintf(out, "    ");
    }
    char buffer[128] = {0};
    if (tree->config.print) {
        tree->config.print(node->payload, tree->config.user_ctx, buffer, sizeof(buffer));
    } else {
        tree_print_value_default(node->payload, NULL, buffer, sizeof(buffer));
    }
    if (tree->type == TREE_TYPE_RB) {
        fprintf(out, "%s [%s]\n", buffer, node->color == RB_RED ? "RED" : "BLACK");
    } else {
        fprintf(out, "%s\n", buffer);
    }
    tree_dump_text_node(tree, node->left, out, depth + 1);
    tree_dump_text_node(tree, node->right, out, depth + 1);
    return 0;
}

static void tree_dump_dot_node(const tree_t* tree,
                               const tree_node_t* node,
                               FILE* out) {
    if (node == NULL || node == tree->nil) {
        return;
    }
    char value[128] = {0};
    if (tree->config.print) {
        tree->config.print(node->payload, tree->config.user_ctx, value, sizeof(value));
    } else {
        tree_print_value_default(node->payload, NULL, value, sizeof(value));
    }
    fprintf(out, "    \"%p\" [label=\"%s\"%s];\n",
            (void*)node,
            value,
            tree->type == TREE_TYPE_RB ? (node->color == RB_RED ? ", color=red" : ", color=black") : "");
    if (node->left && node->left != tree->nil) {
        fprintf(out, "    \"%p\" -> \"%p\";\n", (void*)node, (void*)node->left);
    }
    if (node->right && node->right != tree->nil) {
        fprintf(out, "    \"%p\" -> \"%p\";\n", (void*)node, (void*)node->right);
    }
    tree_dump_dot_node(tree, node->left, out);
    tree_dump_dot_node(tree, node->right, out);
}

static tree_status_t tree_validate_tree(const tree_t* tree) {
    if (tree == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    switch (tree->type) {
        case TREE_TYPE_BST:
            return TREE_OK;
        case TREE_TYPE_AVL:
            return avl_validate(tree);
        case TREE_TYPE_RB:
            return rb_validate(tree);
        default:
            return TREE_OK;
    }
}

/* Public API */

tree_status_t tree_validate(const tree_t* tree) {
    return tree_validate_tree(tree);
}

tree_status_t tree_dump_text(const tree_t* tree, FILE* out) {
    if (tree == NULL || out == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    if (tree->root == NULL || tree->root == tree->nil) {
        fprintf(out, "(empty tree)\n");
        return TREE_OK;
    }
    tree_dump_text_node(tree, tree->root, out, 0);
    return TREE_OK;
}

tree_status_t tree_dump_dot(const tree_t* tree, FILE* out) {
    if (tree == NULL || out == NULL) {
        return TREE_ERR_INVALID_ARGUMENT;
    }
    fprintf(out, "digraph Tree {\n");
    fprintf(out, "    node [shape=record, style=filled, fillcolor=white];\n");
    if (tree->root != NULL && tree->root != tree->nil) {
        tree_dump_dot_node(tree, tree->root, out);
    }
    fprintf(out, "}\n");
    return TREE_OK;
}
