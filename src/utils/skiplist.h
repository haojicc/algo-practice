/**
 * @file skiplist.h
 * @brief Skip list implementation used by ZSet sorted set.
 *
 * The skip list stores ordered members by score, with stable ordering for
 * duplicate scores using a member comparator.
 */

#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compare callback for skip list members.
 */
typedef int (*ZSetCompareFn)(const void* a, const void* b, size_t member_size);

/**
 * @brief Skip list level node representation.
 */
typedef struct ZSkipListLevel {
    struct ZSkipListNode* forward;
    unsigned int span;
} ZSkipListLevel;

/**
 * @brief Node stored in the skip list.
 */
typedef struct ZSkipListNode {
    void* member;
    double score;
    struct ZSkipListNode* backward;
    ZSkipListLevel level[];
} ZSkipListNode;

/**
 * @brief Sorted skip list container.
 */
typedef struct ZSkipList {
    ZSkipListNode* header;
    ZSkipListNode* tail;
    unsigned long length;
    int level;
    int max_level;
    double p;
    ZSetCompareFn compare;
    size_t member_size;
} ZSkipList;

ZSkipList* skiplist_create(int max_level,
                           double p,
                           ZSetCompareFn compare,
                           size_t member_size);
void skiplist_destroy(ZSkipList* sl,
                      void (*free_member)(void* member));
ZSkipListNode* skiplist_insert(ZSkipList* sl,
                               void* member,
                               double score);
bool skiplist_delete(ZSkipList* sl,
                     const void* member,
                     double score);
ZSkipListNode* skiplist_search(ZSkipList* sl,
                               const void* member,
                               double score);
bool skiplist_update_score(ZSkipList* sl,
                           ZSkipListNode* node,
                           double new_score);
unsigned long skiplist_get_rank(ZSkipList* sl,
                                const void* member,
                                double score);
ZSkipListNode* skiplist_get_element_by_rank(ZSkipList* sl,
                                            unsigned long rank);
ZSkipListNode* skiplist_first_in_score_range(ZSkipList* sl,
                                             double min,
                                             double max);
ZSkipListNode* skiplist_last_in_score_range(ZSkipList* sl,
                                            double min,
                                            double max);

#ifdef __cplusplus
}
#endif

#endif /* SKIPLIST_H */
