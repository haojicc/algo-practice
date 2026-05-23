/**
 * @file test_tree.c
 * @brief Unit tests for generic tree implementations
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tree_base.h"
#include "binary_tree.h"
#include "bst.h"
#include "avl_tree.h"
#include "rb_tree.h"
#include "debug.h"

TREE_DECLARE(int_tree, int)

static int int_compare(const void* lhs, const void* rhs, void* user_ctx) {
    (void)user_ctx;
    int a = *(const int*)lhs;
    int b = *(const int*)rhs;
    return (a > b) - (a < b);
}

static void int_print(const void* value, void* user_ctx, char* buffer, size_t size) {
    (void)user_ctx;
    if (buffer && value) {
        snprintf(buffer, size, "%d", *(const int*)value);
    }
}

static void populate_ordered(tree_t* tree, int start, int end) {
    for (int i = start; i <= end; ++i) {
        tree_insert(tree, &i);
    }
}

static void test_tree_ordered(const char* name, tree_t* tree) {
    printf("-- %s ordered insert / remove / search --\n", name);
    for (int i = 1; i <= 10; ++i) {
        tree_status_t status = tree_insert(tree, &i);
        if (status != TREE_OK) {
            printf("Failed insert %d\n", i);
            return;
        }
    }
    for (int i = 1; i <= 10; ++i) {
        if (!tree_contains(tree, &i)) {
            printf("Missing value %d\n", i);
            return;
        }
    }
    int key = 5;
    tree_remove(tree, &key);
    if (tree_contains(tree, &key)) {
        printf("Remove failed for %d\n", key);
        return;
    }
    printf("Count after remove: %zu\n", tree_size(tree));
}

static void test_binary_tree(void) {
    printf("Testing Binary Tree\n");
    tree_t tree;
    tree_config_t config = {int_compare, NULL, NULL, int_print, NULL, NULL, NULL};
    if (bt_init(&tree, &config, sizeof(int)) != TREE_OK) {
        printf("Failed binary tree init\n");
        return;
    }

    int values[] = {7, 3, 11, 1, 5};
    bt_insert_left(&tree, NULL, &values[0]);
    bt_insert_left(&tree, tree.root, &values[1]);
    bt_insert_right(&tree, tree.root, &values[2]);
    bt_insert_left(&tree, tree.root->left, &values[3]);
    bt_insert_right(&tree, tree.root->left, &values[4]);

    printf("Binary tree size: %zu\n", bt_node_count(&tree));
    printf("Binary tree leaf count: %zu\n", bt_leaf_count(&tree));
    tree_dump_text(&tree, stdout);
    tree_destroy(&tree);
}

static void test_bst(void) {
    printf("Testing BST\n");
    int_tree_t tree;
    tree_config_t config = {int_compare, NULL, NULL, int_print, NULL, NULL, NULL};
    int_tree_init(&tree, TREE_TYPE_BST, &config);
    test_tree_ordered("BST", &tree.base);
    tree_destroy(&tree.base);
}

static void test_avl(void) {
    printf("Testing AVL Tree\n");
    int_tree_t tree;
    tree_config_t config = {int_compare, NULL, NULL, int_print, NULL, NULL, NULL};
    int_tree_init(&tree, TREE_TYPE_AVL, &config);
    test_tree_ordered("AVL", &tree.base);
    if (avl_validate(&tree.base) != TREE_OK) {
        printf("AVL validation failed\n");
    }
    tree_destroy(&tree.base);
}

static void test_rb(void) {
    printf("Testing Red-Black Tree\n");
    int_tree_t tree;
    tree_config_t config = {int_compare, NULL, NULL, int_print, NULL, NULL, NULL};
    int_tree_init(&tree, TREE_TYPE_RB, &config);
    test_tree_ordered("RB", &tree.base);
    if (rb_validate(&tree.base) != TREE_OK) {
        printf("RB validation failed\n");
    }
    tree_destroy(&tree.base);
}

void test_tree(void) {
    test_binary_tree();
    test_bst();
    test_avl();
    test_rb();
    printf("Tree tests completed\n");
}
