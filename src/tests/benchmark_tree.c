#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tree_base.h"
#include "bst.h"
#include "avl_tree.h"
#include "rb_tree.h"

static int int_compare(const void* lhs, const void* rhs, void* user_ctx) {
    (void)user_ctx;
    int a = *(const int*)lhs;
    int b = *(const int*)rhs;
    return (a > b) - (a < b);
}

static void benchmark_tree(tree_type_t type, const char* name, int count) {
    tree_t tree;
    tree_config_t config = {int_compare, NULL, NULL, NULL, NULL, NULL, NULL};
    tree_init(&tree, type, &config, sizeof(int));

    clock_t start = clock();
    for (int i = 0; i < count; ++i) {
        int value = rand();
        tree_insert(&tree, &value);
    }
    clock_t mid = clock();
    for (int i = 0; i < count; ++i) {
        int value = rand();
        tree_search(&tree, &value);
    }
    clock_t end = clock();

    double insert_time = (double)(mid - start) / CLOCKS_PER_SEC;
    double search_time = (double)(end - mid) / CLOCKS_PER_SEC;
    printf("%s %d ops: insert=%.3fs search=%.3fs size=%zu\n", name, count, insert_time, search_time, tree_size(&tree));
    tree_destroy(&tree);
}

int main(void) {
    srand((unsigned int)time(NULL));
    int sizes[] = {1000, 100000, 1000000};
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        benchmark_tree(TREE_TYPE_BST, "BST", sizes[i]);
        benchmark_tree(TREE_TYPE_AVL, "AVL", sizes[i]);
        benchmark_tree(TREE_TYPE_RB, "RBT", sizes[i]);
        printf("---\n");
    }
    return 0;
}
