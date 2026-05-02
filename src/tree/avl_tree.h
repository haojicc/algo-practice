#ifndef AVL_TREE_H
#define AVL_TREE_H

#include <stdio.h>
#include <stdlib.h>

// AVL树节点
typedef struct Node {
    int key;
    struct Node *left;
    struct Node *right;
    int height;
} Node;

// ===== 对外暴露的接口 =====

// 插入
Node* avl_insert(Node* root, int key);

// 删除
Node* avl_delete(Node* root, int key);

// 前序遍历
void avl_preorder(Node* root);

#endif