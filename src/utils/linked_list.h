/**
 * @file linked_list.h
 * @brief Generic linked list library for singly, doubly, and circular lists.
 *
 * This header defines a production-grade, generic linked list API that
 * stores arbitrary element values by copy. The implementation supports
 * optional comparator and destructor callbacks, iterator traversal, sorting,
 * reversing, merging, splitting, unique filtering, and list/array conversion.
 *
 * The goal is high maintainability, safe memory management, and portable
 * C11 compatibility for Linux/macOS.
 */

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Supported linked list types.
 */
typedef enum ListType {
    LIST_TYPE_SINGLY = 0,
    LIST_TYPE_DOUBLY,
    LIST_TYPE_SINGLY_CIRCULAR,
    LIST_TYPE_DOUBLY_CIRCULAR
} ListType;

/**
 * @brief Uniform error codes returned by linked list operations.
 */
typedef enum ListResult {
    LIST_OK = 0,
    LIST_ERR_NULL,
    LIST_ERR_ALLOC,
    LIST_ERR_OUT_OF_RANGE,
    LIST_ERR_EMPTY,
    LIST_ERR_NOT_FOUND,
    LIST_ERR_INVALID_OPERATION
} ListResult;

/**
 * @brief Comparison callback used by search, sort, and unique operations.
 * @return negative when a < b, zero when equal, positive when a > b.
 */
typedef int (*ListCompareFn)(const void* a, const void* b);

/**
 * @brief Destructor callback for elements stored inside the list.
 */
typedef void (*ListDestroyFn)(void* data);

/**
 * @brief Clone callback used to deep copy element payloads.
 * @return newly allocated copy of the element or NULL on failure.
 */
typedef void* (*ListCloneFn)(const void* data);

/**
 * @brief Single list node.
 */
typedef struct ListNode {
    void* data;
    size_t data_size;
    struct ListNode* next;
    struct ListNode* prev;
} ListNode;

/**
 * @brief Generic linked list control structure.
 */
typedef struct List {
    ListNode* head;
    ListNode* tail;
    size_t size;
    size_t element_size;
    ListType type;
    ListCompareFn compare;
    ListDestroyFn destroy;
    ListCloneFn clone;
} List;

/**
 * @brief Forward iterator for traversing a list without exposing internals.
 */
typedef struct ListIterator {
    const List* list;
    const ListNode* current;
    size_t visited;
} ListIterator;

/* Creation and destruction */
List* list_create(size_t element_size, ListType type);
List* list_create_with_callbacks(size_t element_size,
                                 ListType type,
                                 ListCompareFn compare,
                                 ListDestroyFn destroy,
                                 ListCloneFn clone);
void list_destroy(List** list_ptr);
ListResult list_clear(List* list);

/* Insertion */
ListResult list_push_front(List* list, const void* element);
ListResult list_push_back(List* list, const void* element);
ListResult list_insert_at(List* list, size_t index, const void* element);
ListResult list_insert_after(List* list, ListNode* node, const void* element);
ListResult list_insert_before(List* list, ListNode* node, const void* element);

/* Removal */
ListResult list_pop_front(List* list, void* out_element);
ListResult list_pop_back(List* list, void* out_element);
ListResult list_remove_at(List* list, size_t index, void* out_element);
ListResult list_remove_value(List* list, const void* element);
ListResult list_remove_node(List* list, ListNode* node);

/* Query */
ListResult list_get(const List* list, size_t index, void* out_element);
const void* list_front(const List* list);
const void* list_back(const List* list);
ListNode* list_find(const List* list, const void* element);
bool list_contains(const List* list, const void* element);

/* Status */
bool list_is_empty(const List* list);
size_t list_size(const List* list);

/* Traversal */
ListResult list_foreach(const List* list,
                        void (*visit)(const void* element, size_t index, void* user_data),
                        void* user_data);
void list_iterator_init(const List* list, ListIterator* iterator);
bool list_iterator_has_next(const ListIterator* iterator);
const void* list_iterator_next(ListIterator* iterator);

/* Advanced operations */
ListResult list_sort(List* list, ListCompareFn compare);
ListResult list_reverse(List* list);
List* list_shallow_copy(const List* source);
List* list_deep_copy(const List* source);
ListResult list_merge(List* destination, List* source);
ListResult list_split(List* source, size_t index, List** first, List** second);
ListResult list_unique(List* list);

/* Conversion helpers */
List* array_to_list(const void* array,
                    size_t count,
                    size_t element_size,
                    ListType type,
                    ListCompareFn compare,
                    ListDestroyFn destroy,
                    ListCloneFn clone);
ListResult list_to_array(const List* list, void* out_array, size_t max_count);

#ifdef __cplusplus
}
#endif

#endif /* LINKED_LIST_H */
