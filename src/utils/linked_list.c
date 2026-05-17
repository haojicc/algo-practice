/**
 * @file linked_list.c
 * @brief Implementation of a generic linked list library.
 *
 * This implementation supports singly linked lists, doubly linked lists,
 * and circular variants. It stores element payloads by copy and provides
 * optional comparison, destruction, and clone callbacks for advanced usage.
 */

#include "linked_list.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

static bool list_is_valid(const List* list) {
    return list != NULL && list->element_size > 0;
}

static bool list_is_circular(const List* list) {
    return list != NULL && (list->type == LIST_TYPE_SINGLY_CIRCULAR || list->type == LIST_TYPE_DOUBLY_CIRCULAR);
}

static bool list_is_doubly(const List* list) {
    return list != NULL && (list->type == LIST_TYPE_DOUBLY || list->type == LIST_TYPE_DOUBLY_CIRCULAR);
}

static void* copy_element(const List* list, const void* element) {
    if (element == NULL) {
        return NULL;
    }

    if (list != NULL && list->clone != NULL) {
        return list->clone(element);
    }

    void* copy = malloc(list->element_size);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, element, list->element_size);
    return copy;
}

static int compare_element(const List* list, const void* a, const void* b) {
    if (list != NULL && list->compare != NULL) {
        return list->compare(a, b);
    }
    return memcmp(a, b, list->element_size);
}

static void destroy_data(const List* list, void* data) {
    if (data == NULL) {
        return;
    }
    if (list != NULL && list->destroy != NULL) {
        list->destroy(data);
    }
    free(data);
}

static ListNode* create_node(const List* list, const void* element) {
    if (list == NULL || element == NULL) {
        return NULL;
    }

    ListNode* node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }

    node->data = malloc(list->element_size);
    if (node->data == NULL) {
        free(node);
        return NULL;
    }

    memcpy(node->data, element, list->element_size);
    node->data_size = list->element_size;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

static ListNode* create_node_with_copy(const List* list, const void* element) {
    if (list == NULL || element == NULL) {
        return NULL;
    }

    ListNode* node = malloc(sizeof(*node));
    if (node == NULL) {
        return NULL;
    }

    node->data = copy_element(list, element);
    if (node->data == NULL) {
        free(node);
        return NULL;
    }

    node->data_size = list->element_size;
    node->next = NULL;
    node->prev = NULL;
    return node;
}

static void break_circular(List* list) {
    if (list == NULL || !list_is_circular(list) || list->size == 0) {
        return;
    }
    if (list->tail != NULL) {
        list->tail->next = NULL;
    }
    if (list_is_doubly(list) && list->head != NULL) {
        list->head->prev = NULL;
    }
}

static void restore_circular(List* list) {
    if (list == NULL || !list_is_circular(list) || list->size == 0) {
        return;
    }
    if (list->tail != NULL && list->head != NULL) {
        list->tail->next = list->head;
        if (list_is_doubly(list)) {
            list->head->prev = list->tail;
        }
    }
}

List* list_create(size_t element_size, ListType type) {
    return list_create_with_callbacks(element_size, type, NULL, NULL, NULL);
}

List* list_create_with_callbacks(size_t element_size,
                                 ListType type,
                                 ListCompareFn compare,
                                 ListDestroyFn destroy,
                                 ListCloneFn clone) {
    if (element_size == 0) {
        return NULL;
    }
    if (type < LIST_TYPE_SINGLY || type > LIST_TYPE_DOUBLY_CIRCULAR) {
        return NULL;
    }

    List* list = malloc(sizeof(*list));
    if (list == NULL) {
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    list->element_size = element_size;
    list->type = type;
    list->compare = compare;
    list->destroy = destroy;
    list->clone = clone;
    return list;
}

void list_destroy(List** list_ptr) {
    if (list_ptr == NULL || *list_ptr == NULL) {
        return;
    }
    list_clear(*list_ptr);
    free(*list_ptr);
    *list_ptr = NULL;
}

ListResult list_clear(List* list) {
    if (list == NULL) {
        return LIST_ERR_NULL;
    }

    ListNode* node = list->head;
    size_t count = list->size;

    while (count-- > 0 && node != NULL) {
        ListNode* next = node->next;
        destroy_data(list, node->data);
        free(node);
        node = next;
        if (list_is_circular(list) && node == list->head) {
            break;
        }
    }

    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
    return LIST_OK;
}

static ListResult attach_node(List* list, ListNode* node, bool at_front) {
    if (list == NULL || node == NULL) {
        return LIST_ERR_NULL;
    }

    if (list->size == 0) {
        list->head = node;
        list->tail = node;
        if (list_is_circular(list)) {
            node->next = node;
            if (list_is_doubly(list)) {
                node->prev = node;
            }
        }
    } else if (at_front) {
        node->next = list->head;
        node->prev = list_is_doubly(list) ? list->head->prev : NULL;
        if (list_is_doubly(list)) {
            list->head->prev = node;
        }
        list->head = node;
        if (list_is_circular(list)) {
            list->tail->next = list->head;
            if (list_is_doubly(list)) {
                list->head->prev = list->tail;
            }
        }
    } else {
        node->prev = list_is_doubly(list) ? list->tail : NULL;
        node->next = list_is_circular(list) ? list->head : NULL;
        list->tail->next = node;
        if (list_is_doubly(list)) {
            list->tail->next = node;
            node->prev = list->tail;
        }
        list->tail = node;
        if (list_is_circular(list) && list_is_doubly(list)) {
            list->head->prev = list->tail;
        }
    }

    list->size += 1;
    return LIST_OK;
}

static ListNode* node_at_index(const List* list, size_t index) {
    if (!list_is_valid(list) || index >= list->size) {
        return NULL;
    }

    const bool circular = list_is_circular(list);
    ListNode* node = list->head;
    size_t i = 0;

    while (i < index && node != NULL) {
        node = node->next;
        if (circular && node == list->head) {
            break;
        }
        ++i;
    }
    return node;
}

ListResult list_push_front(List* list, const void* element) {
    if (!list_is_valid(list) || element == NULL) {
        return LIST_ERR_NULL;
    }

    ListNode* node = create_node_with_copy(list, element);
    if (node == NULL) {
        return LIST_ERR_ALLOC;
    }
    return attach_node(list, node, true);
}

ListResult list_push_back(List* list, const void* element) {
    if (!list_is_valid(list) || element == NULL) {
        return LIST_ERR_NULL;
    }

    ListNode* node = create_node_with_copy(list, element);
    if (node == NULL) {
        return LIST_ERR_ALLOC;
    }
    return attach_node(list, node, false);
}

ListResult list_insert_at(List* list, size_t index, const void* element) {
    if (!list_is_valid(list) || element == NULL) {
        return LIST_ERR_NULL;
    }
    if (index > list->size) {
        return LIST_ERR_OUT_OF_RANGE;
    }
    if (index == 0) {
        return list_push_front(list, element);
    }
    if (index == list->size) {
        return list_push_back(list, element);
    }

    ListNode* next_node = node_at_index(list, index);
    if (next_node == NULL) {
        return LIST_ERR_OUT_OF_RANGE;
    }

    ListNode* new_node = create_node_with_copy(list, element);
    if (new_node == NULL) {
        return LIST_ERR_ALLOC;
    }

    ListNode* prev_node = NULL;
    if (list_is_doubly(list)) {
        prev_node = next_node->prev;
    } else {
        prev_node = list->head;
        while (prev_node != NULL && prev_node->next != next_node) {
            prev_node = prev_node->next;
            if (list_is_circular(list) && prev_node == list->head) {
                break;
            }
        }
    }
    if (prev_node == NULL) {
        free(new_node->data);
        free(new_node);
        return LIST_ERR_INVALID_OPERATION;
    }

    new_node->next = next_node;
    new_node->prev = prev_node;
    next_node->prev = new_node;
    prev_node->next = new_node;

    if (list_is_circular(list) && list->tail != NULL) {
        list->tail->next = list->head;
        if (list_is_doubly(list)) {
            list->head->prev = list->tail;
        }
    }

    list->size += 1;
    return LIST_OK;
}

ListResult list_insert_after(List* list, ListNode* node, const void* element) {
    if (!list_is_valid(list) || node == NULL || element == NULL) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }

    ListNode* current = list->head;
    bool found = false;
    size_t remaining = list->size;
    while (remaining-- > 0 && current != NULL) {
        if (current == node) {
            found = true;
            break;
        }
        current = current->next;
    }
    if (!found) {
        return LIST_ERR_NOT_FOUND;
    }

    if (current == list->tail) {
        return list_push_back(list, element);
    }

    ListNode* new_node = create_node_with_copy(list, element);
    if (new_node == NULL) {
        return LIST_ERR_ALLOC;
    }

    new_node->next = current->next;
    new_node->prev = current;
    if (list_is_doubly(list) && current->next != NULL) {
        current->next->prev = new_node;
    }
    current->next = new_node;
    list->size += 1;
    return LIST_OK;
}

ListResult list_insert_before(List* list, ListNode* node, const void* element) {
    if (!list_is_valid(list) || node == NULL || element == NULL) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }
    if (node == list->head) {
        return list_push_front(list, element);
    }

    if (list_is_doubly(list)) {
        ListNode* prev_node = node->prev;
        if (prev_node == NULL) {
            return LIST_ERR_INVALID_OPERATION;
        }
        return list_insert_after(list, prev_node, element);
    }

    /* Singly list must search predecessor */
    ListNode* current = list->head;
    while (current != NULL && current->next != node) {
        current = current->next;
        if (list_is_circular(list) && current == list->head) {
            break;
        }
    }
    if (current == NULL || current->next != node) {
        return LIST_ERR_NOT_FOUND;
    }
    return list_insert_after(list, current, element);
}

static ListResult extract_node(List* list, ListNode* node, void* out_element) {
    if (!list_is_valid(list) || node == NULL) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }

    if (out_element != NULL) {
        memcpy(out_element, node->data, list->element_size);
    }

    if (list->size == 1) {
        list->head = NULL;
        list->tail = NULL;
    } else if (list_is_doubly(list)) {
        if (node == list->head) {
            list->head = node->next;
        }
        if (node == list->tail) {
            list->tail = node->prev;
        }
        if (node->prev != NULL) {
            node->prev->next = node->next;
        }
        if (node->next != NULL) {
            node->next->prev = node->prev;
        }
        if (list_is_circular(list)) {
            list->tail->next = list->head;
            list->head->prev = list->tail;
        }
    } else {
        if (node == list->head) {
            list->head = node->next;
        } else {
            ListNode* predecessor = list->head;
            while (predecessor != NULL && predecessor->next != node) {
                predecessor = predecessor->next;
                if (list_is_circular(list) && predecessor == list->head) {
                    break;
                }
            }
            if (predecessor != NULL) {
                predecessor->next = node->next;
            }
        }
        if (node == list->tail) {
            ListNode* predecessor = list->head;
            while (predecessor != NULL && predecessor->next != node) {
                predecessor = predecessor->next;
                if (list_is_circular(list) && predecessor == list->head) {
                    break;
                }
            }
            list->tail = predecessor;
        }
        if (list_is_circular(list) && list->tail != NULL) {
            list->tail->next = list->head;
        }
    }

    destroy_data(list, node->data);
    free(node);
    list->size -= 1;
    return LIST_OK;
}

ListResult list_pop_front(List* list, void* out_element) {
    if (!list_is_valid(list)) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }
    return extract_node(list, list->head, out_element);
}

ListResult list_pop_back(List* list, void* out_element) {
    if (!list_is_valid(list)) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }
    return extract_node(list, list->tail, out_element);
}

ListResult list_remove_at(List* list, size_t index, void* out_element) {
    if (!list_is_valid(list)) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }
    if (index >= list->size) {
        return LIST_ERR_OUT_OF_RANGE;
    }
    ListNode* node = node_at_index(list, index);
    if (node == NULL) {
        return LIST_ERR_OUT_OF_RANGE;
    }
    return extract_node(list, node, out_element);
}

ListResult list_remove_value(List* list, const void* element) {
    if (!list_is_valid(list) || element == NULL) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }

    ListNode* node = list_find(list, element);
    if (node == NULL) {
        return LIST_ERR_NOT_FOUND;
    }
    return extract_node(list, node, NULL);
}

ListResult list_remove_node(List* list, ListNode* node) {
    if (!list_is_valid(list) || node == NULL) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }

    ListNode* current = list->head;
    size_t remaining = list->size;
    bool found = false;
    while (remaining-- > 0 && current != NULL) {
        if (current == node) {
            found = true;
            break;
        }
        current = current->next;
    }
    if (!found) {
        return LIST_ERR_NOT_FOUND;
    }
    return extract_node(list, node, NULL);
}

ListResult list_get(const List* list, size_t index, void* out_element) {
    if (!list_is_valid(list) || out_element == NULL) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_ERR_EMPTY;
    }
    if (index >= list->size) {
        return LIST_ERR_OUT_OF_RANGE;
    }

    ListNode* node = node_at_index(list, index);
    if (node == NULL) {
        return LIST_ERR_OUT_OF_RANGE;
    }
    memcpy(out_element, node->data, list->element_size);
    return LIST_OK;
}

const void* list_front(const List* list) {
    if (!list_is_valid(list) || list->size == 0) {
        return NULL;
    }
    return list->head->data;
}

const void* list_back(const List* list) {
    if (!list_is_valid(list) || list->size == 0) {
        return NULL;
    }
    return list->tail->data;
}

ListNode* list_find(const List* list, const void* element) {
    if (!list_is_valid(list) || element == NULL) {
        return NULL;
    }
    if (list->size == 0) {
        return NULL;
    }

    ListNode* node = list->head;
    size_t remaining = list->size;
    while (remaining-- > 0 && node != NULL) {
        if (compare_element(list, node->data, element) == 0) {
            return node;
        }
        node = node->next;
        if (list_is_circular(list) && node == list->head) {
            break;
        }
    }
    return NULL;
}

bool list_contains(const List* list, const void* element) {
    return list_find(list, element) != NULL;
}

bool list_is_empty(const List* list) {
    return list == NULL || list->size == 0;
}

size_t list_size(const List* list) {
    return list != NULL ? list->size : 0;
}

ListResult list_foreach(const List* list,
                        void (*visit)(const void* element, size_t index, void* user_data),
                        void* user_data) {
    if (!list_is_valid(list) || visit == NULL) {
        return LIST_ERR_NULL;
    }
    if (list->size == 0) {
        return LIST_OK;
    }

    const ListNode* node = list->head;
    size_t index = 0;
    size_t remaining = list->size;

    while (remaining-- > 0 && node != NULL) {
        visit(node->data, index, user_data);
        node = node->next;
        ++index;
        if (list_is_circular(list) && node == list->head) {
            break;
        }
    }
    return LIST_OK;
}

void list_iterator_init(const List* list, ListIterator* iterator) {
    if (iterator == NULL) {
        return;
    }
    iterator->list = list;
    iterator->current = list != NULL ? list->head : NULL;
    iterator->visited = 0;
}

bool list_iterator_has_next(const ListIterator* iterator) {
    return iterator != NULL && iterator->list != NULL && iterator->visited < iterator->list->size && iterator->current != NULL;
}

const void* list_iterator_next(ListIterator* iterator) {
    if (!list_iterator_has_next(iterator)) {
        return NULL;
    }

    const void* data = iterator->current->data;
    iterator->current = iterator->current->next;
    iterator->visited += 1;
    return data;
}

static ListNode* merge_sorted_nodes(List* list, ListNode* left, ListNode* right, ListCompareFn compare) {
    ListNode dummy = {0};
    ListNode* tail = &dummy;
    while (left != NULL && right != NULL) {
        int order = compare != NULL ? compare(left->data, right->data) : compare_element(list, left->data, right->data);
        if (order <= 0) {
            tail->next = left;
            left->prev = tail == &dummy ? NULL : tail;
            left = left->next;
        } else {
            tail->next = right;
            right->prev = tail == &dummy ? NULL : tail;
            right = right->next;
        }
        tail = tail->next;
    }
    tail->next = left != NULL ? left : right;
    if (tail->next != NULL) {
        tail->next->prev = tail;
    }
    return dummy.next;
}

static ListNode* split_list(ListNode* head) {
    ListNode* slow = head;
    ListNode* fast = head->next;
    while (fast != NULL && fast->next != NULL) {
        slow = slow->next;
        fast = fast->next->next;
    }
    ListNode* second = slow->next;
    slow->next = NULL;
    if (second != NULL) {
        second->prev = NULL;
    }
    return second;
}

static ListNode* merge_sort_nodes(List* list, ListNode* head, ListCompareFn compare) {
    if (head == NULL || head->next == NULL) {
        return head;
    }
    ListNode* second = split_list(head);
    head = merge_sort_nodes(list, head, compare);
    second = merge_sort_nodes(list, second, compare);
    return merge_sorted_nodes(list, head, second, compare);
}

static void rebuild_tail(List* list) {
    if (list == NULL || list->head == NULL) {
        list->tail = NULL;
        return;
    }
    ListNode* node = list->head;
    while (node->next != NULL) {
        node = node->next;
    }
    list->tail = node;
}

ListResult list_sort(List* list, ListCompareFn compare) {
    if (!list_is_valid(list)) {
        return LIST_ERR_NULL;
    }
    if (list->size < 2) {
        return LIST_OK;
    }

    bool was_circular = list_is_circular(list);
    if (was_circular) {
        break_circular(list);
    }

    list->head = merge_sort_nodes(list, list->head, compare != NULL ? compare : list->compare);
    rebuild_tail(list);

    if (list_is_doubly(list)) {
        ListNode* previous = NULL;
        ListNode* current = list->head;
        while (current != NULL) {
            current->prev = previous;
            previous = current;
            current = current->next;
        }
    }
    if (was_circular) {
        restore_circular(list);
    }
    return LIST_OK;
}

ListResult list_reverse(List* list) {
    if (!list_is_valid(list)) {
        return LIST_ERR_NULL;
    }
    if (list->size < 2) {
        return LIST_OK;
    }

    bool was_circular = list_is_circular(list);
    if (was_circular) {
        break_circular(list);
    }

    ListNode* current = list->head;
    ListNode* prev = NULL;
    while (current != NULL) {
        ListNode* next = current->next;
        current->next = prev;
        current->prev = next;
        prev = current;
        current = next;
    }

    ListNode* old_head = list->head;
    list->head = list->tail;
    list->tail = old_head;

    if (list_is_doubly(list) && list->head != NULL) {
        ListNode* walker = list->head;
        ListNode* previous = NULL;
        while (walker != NULL) {
            walker->prev = previous;
            previous = walker;
            walker = walker->next;
        }
    }

    if (was_circular) {
        restore_circular(list);
    }
    return LIST_OK;
}

List* list_shallow_copy(const List* source) {
    if (!list_is_valid(source)) {
        return NULL;
    }
    List* copy = list_create_with_callbacks(source->element_size,
                                            source->type,
                                            source->compare,
                                            source->destroy,
                                            source->clone);
    if (copy == NULL) {
        return NULL;
    }

    ListNode* node = source->head;
    size_t remaining = source->size;
    while (remaining-- > 0 && node != NULL) {
        ListNode* new_node = create_node(source, node->data);
        if (new_node == NULL) {
            list_destroy(&copy);
            return NULL;
        }
        if (attach_node(copy, new_node, false) != LIST_OK) {
            destroy_data(source, new_node->data);
            free(new_node);
            list_destroy(&copy);
            return NULL;
        }
        node = node->next;
        if (list_is_circular(source) && node == source->head) {
            break;
        }
    }
    return copy;
}

List* list_deep_copy(const List* source) {
    if (!list_is_valid(source)) {
        return NULL;
    }
    if (source->clone == NULL) {
        return list_shallow_copy(source);
    }

    List* copy = list_create_with_callbacks(source->element_size,
                                            source->type,
                                            source->compare,
                                            source->destroy,
                                            source->clone);
    if (copy == NULL) {
        return NULL;
    }

    ListNode* node = source->head;
    size_t remaining = source->size;
    while (remaining-- > 0 && node != NULL) {
        void* duplicated = source->clone(node->data);
        if (duplicated == NULL) {
            list_destroy(&copy);
            return NULL;
        }
        ListNode* new_node = create_node(source, duplicated);
        free(duplicated);
        if (new_node == NULL) {
            list_destroy(&copy);
            return NULL;
        }
        if (attach_node(copy, new_node, false) != LIST_OK) {
            destroy_data(source, new_node->data);
            free(new_node);
            list_destroy(&copy);
            return NULL;
        }
        node = node->next;
        if (list_is_circular(source) && node == source->head) {
            break;
        }
    }
    return copy;
}

ListResult list_merge(List* destination, List* source) {
    if (!list_is_valid(destination) || !list_is_valid(source)) {
        return LIST_ERR_NULL;
    }
    if (destination == source) {
        return LIST_ERR_INVALID_OPERATION;
    }
    if (destination->type != source->type || destination->element_size != source->element_size) {
        return LIST_ERR_INVALID_OPERATION;
    }

    if (source->size == 0) {
        return LIST_OK;
    }
    if (destination->size == 0) {
        *destination = *source;
        source->head = NULL;
        source->tail = NULL;
        source->size = 0;
        return LIST_OK;
    }

    if (list_is_circular(destination) || list_is_circular(source)) {
        break_circular(destination);
        break_circular(source);
    }

    destination->tail->next = source->head;
    if (list_is_doubly(destination)) {
        source->head->prev = destination->tail;
    }
    destination->tail = source->tail;
    destination->size += source->size;

    if (list_is_circular(destination)) {
        restore_circular(destination);
    }

    source->head = NULL;
    source->tail = NULL;
    source->size = 0;
    return LIST_OK;
}

ListResult list_split(List* source, size_t index, List** first, List** second) {
    if (!list_is_valid(source) || first == NULL || second == NULL) {
        return LIST_ERR_NULL;
    }
    if (index > source->size) {
        return LIST_ERR_OUT_OF_RANGE;
    }

    *first = list_create_with_callbacks(source->element_size,
                                        source->type,
                                        source->compare,
                                        source->destroy,
                                        source->clone);
    *second = list_create_with_callbacks(source->element_size,
                                         source->type,
                                         source->compare,
                                         source->destroy,
                                         source->clone);
    if (*first == NULL || *second == NULL) {
        list_destroy(first);
        list_destroy(second);
        return LIST_ERR_ALLOC;
    }

    ListNode* current = source->head;
    size_t remaining = source->size;
    size_t position = 0;

    while (remaining-- > 0 && current != NULL) {
        ListNode* next = current->next;
        if (position < index) {
            if (list_push_back(*first, current->data) != LIST_OK) {
                list_destroy(first);
                list_destroy(second);
                return LIST_ERR_ALLOC;
            }
        } else {
            if (list_push_back(*second, current->data) != LIST_OK) {
                list_destroy(first);
                list_destroy(second);
                return LIST_ERR_ALLOC;
            }
        }
        current = next;
        if (list_is_circular(source) && current == source->head) {
            break;
        }
        position += 1;
    }

    list_clear(source);
    return LIST_OK;
}

ListResult list_unique(List* list) {
    if (!list_is_valid(list)) {
        return LIST_ERR_NULL;
    }
    if (list->size < 2) {
        return LIST_OK;
    }

    ListNode* outer = list->head;
    size_t outer_count = list->size;

    while (outer_count-- > 0 && outer != NULL) {
        ListNode* previous = outer;
        ListNode* current = outer->next;
        size_t inner_count = list->size - 1;

        while (inner_count-- > 0 && current != NULL) {
            if (compare_element(list, outer->data, current->data) == 0) {
                ListNode* duplicate = current;
                current = current->next;
                previous->next = duplicate->next;
                if (duplicate == list->tail) {
                    list->tail = previous;
                }
                if (list_is_doubly(list) && duplicate->next != NULL) {
                    duplicate->next->prev = previous;
                }
                destroy_data(list, duplicate->data);
                free(duplicate);
                list->size -= 1;
                continue;
            }
            previous = current;
            current = current->next;
        }

        outer = outer->next;
        if (list_is_circular(list) && outer == list->head) {
            break;
        }
    }

    if (list_is_circular(list) && list->tail != NULL) {
        list->tail->next = list->head;
        if (list_is_doubly(list)) {
            list->head->prev = list->tail;
        }
    }
    return LIST_OK;
}

List* array_to_list(const void* array,
                    size_t count,
                    size_t element_size,
                    ListType type,
                    ListCompareFn compare,
                    ListDestroyFn destroy,
                    ListCloneFn clone) {
    if (array == NULL || count == 0 || element_size == 0) {
        return NULL;
    }

    List* list = list_create_with_callbacks(element_size, type, compare, destroy, clone);
    if (list == NULL) {
        return NULL;
    }

    const unsigned char* data = array;
    for (size_t i = 0; i < count; ++i) {
        if (list_push_back(list, data + i * element_size) != LIST_OK) {
            list_destroy(&list);
            return NULL;
        }
    }
    return list;
}

ListResult list_to_array(const List* list, void* out_array, size_t max_count) {
    if (!list_is_valid(list) || out_array == NULL) {
        return LIST_ERR_NULL;
    }
    if (max_count < list->size) {
        return LIST_ERR_OUT_OF_RANGE;
    }

    unsigned char* destination = out_array;
    ListNode* node = list->head;
    size_t remaining = list->size;
    size_t index = 0;

    while (remaining-- > 0 && node != NULL) {
        memcpy(destination + index * list->element_size, node->data, list->element_size);
        node = node->next;
        ++index;
        if (list_is_circular(list) && node == list->head) {
            break;
        }
    }
    return LIST_OK;
}
