/**
 * @file test_linked_list.c
 * @brief Unit tests and usage examples for the generic linked list library.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/linked_list.h"

static int compare_int(const void* a, const void* b) {
    int lhs = *(const int*)a;
    int rhs = *(const int*)b;
    return lhs - rhs;
}

static void destroy_string(void* data) {
    char* text = *(char**)data;
    free(text);
}

static void* clone_string(const void* data) {
    const char* source = *(const char* const*)data;
    if (source == NULL) {
        return NULL;
    }
    size_t length = strlen(source) + 1;
    char* copy = malloc(length);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, source, length);
    char** container = malloc(sizeof(char*));
    if (container == NULL) {
        free(copy);
        return NULL;
    }
    *container = copy;
    return container;
}

static void test_basic_list_operations(void) {
    List* list = list_create_with_callbacks(sizeof(int), LIST_TYPE_DOUBLY, compare_int, NULL, NULL);
    if (list == NULL) {
        abort();
    }
    if (!list_is_empty(list)) {
        abort();
    }

    int first = 10;
    int second = 20;
    int middle = 15;
    if (list_push_back(list, &first) != LIST_OK) {
        abort();
    }
    if (list_push_front(list, &second) != LIST_OK) {
        abort();
    }
    if (list_insert_at(list, 1, &middle) != LIST_OK) {
        abort();
    }
    if (list_size(list) != 3) {
        abort();
    }

    int result = 0;
    if (list_get(list, 0, &result) != LIST_OK || result != 20) {
        abort();
    }
    if (list_get(list, 1, &result) != LIST_OK || result != 15) {
        abort();
    }
    if (list_get(list, 2, &result) != LIST_OK || result != 10) {
        abort();
    }

    if (!list_contains(list, &middle) || list_find(list, &middle) == NULL) {
        abort();
    }

    if (list_pop_front(list, &result) != LIST_OK || result != 20) {
        abort();
    }
    if (list_pop_back(list, &result) != LIST_OK || result != 10) {
        abort();
    }
    if (list_size(list) != 1) {
        abort();
    }
    list_destroy(&list);
    if (list != NULL) {
        abort();
    }
}

static void test_sort_reverse_merge_unique(void) {
    int input[] = {5, 2, 7, 2, 9};
    List* list = array_to_list(input, 5, sizeof(int), LIST_TYPE_SINGLY, compare_int, NULL, NULL);
    if (list == NULL || list_size(list) != 5) {
        abort();
    }

    if (list_sort(list, NULL) != LIST_OK) {
        abort();
    }

    int expected[] = {2, 2, 5, 7, 9};
    int buffer[5];
    if (list_to_array(list, buffer, 5) != LIST_OK) {
        abort();
    }
    for (size_t i = 0; i < 5; ++i) {
        if (buffer[i] != expected[i]) {
            abort();
        }
    }

    if (list_unique(list) != LIST_OK || list_size(list) != 4) {
        abort();
    }
    if (list_reverse(list) != LIST_OK) {
        abort();
    }
    int reverse_expected[] = {9, 7, 5, 2};
    if (list_to_array(list, buffer, 4) != LIST_OK) {
        abort();
    }
    for (size_t i = 0; i < 4; ++i) {
        if (buffer[i] != reverse_expected[i]) {
            abort();
        }
    }

    list_destroy(&list);
}

static void test_split_copy(void) {
    int values[] = {1, 2, 3, 4};
    List* source = array_to_list(values, 4, sizeof(int), LIST_TYPE_DOUBLY_CIRCULAR, compare_int, NULL, NULL);
    if (source == NULL || list_size(source) != 4) {
        abort();
    }

    List* first = NULL;
    List* second = NULL;
    if (list_split(source, 2, &first, &second) != LIST_OK) {
        abort();
    }
    if (list_size(first) != 2 || list_size(second) != 2 || !list_is_empty(source)) {
        abort();
    }

    int buffer[2];
    if (list_to_array(first, buffer, 2) != LIST_OK || buffer[0] != 1 || buffer[1] != 2) {
        abort();
    }
    if (list_to_array(second, buffer, 2) != LIST_OK || buffer[0] != 3 || buffer[1] != 4) {
        abort();
    }

    list_destroy(&first);
    list_destroy(&second);
    list_destroy(&source);
}

static void test_shallow_deep_copy(void) {
    const char* original[] = {"apple", "banana", "cherry"};
    List* source = array_to_list(original, 3, sizeof(char*), LIST_TYPE_SINGLY, NULL, NULL, NULL);
    assert(source != NULL);

    List* shallow = list_shallow_copy(source);
    assert(shallow != NULL);
    assert(list_size(shallow) == 3);

    List* deep = list_create_with_callbacks(sizeof(char*), LIST_TYPE_SINGLY, NULL, destroy_string, clone_string);
    assert(deep != NULL);
    for (size_t i = 0; i < 3; ++i) {
        char* item = strdup(original[i]);
        assert(item != NULL);
        assert(list_push_back(deep, &item) == LIST_OK);
        free(item);
    }

    List* deep_copy = list_deep_copy(deep);
    assert(deep_copy != NULL);
    assert(list_size(deep_copy) == 3);
    list_destroy(&shallow);
    list_destroy(&deep);
    list_destroy(&deep_copy);
    list_destroy(&source);
}

static void test_iterator_traversal(void) {
    int values[] = {11, 22, 33};
    List* list = array_to_list(values, 3, sizeof(int), LIST_TYPE_DOUBLY, NULL, NULL, NULL);
    if (list == NULL) {
        abort();
    }

    ListIterator it;
    list_iterator_init(list, &it);
    int index = 0;
    while (list_iterator_has_next(&it)) {
        const int* value = list_iterator_next(&it);
        if (value == NULL || *value != values[index++]) {
            abort();
        }
    }
    if (index != 3) {
        abort();
    }
    list_destroy(&list);
}

void test_linked_list(void) {
    printf("--- test_linked_list begin ---\n");
    test_basic_list_operations();
    test_sort_reverse_merge_unique();
    test_split_copy();
    test_shallow_deep_copy();
    test_iterator_traversal();
    printf("--- test_linked_list passed ---\n");
}
