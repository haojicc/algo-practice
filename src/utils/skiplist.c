/**
 * @file skiplist.c
 * @brief Skip list implementation supporting ordered score ranges for ZSet.
 */

#include "skiplist.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int skiplist_random_level(const ZSkipList* sl) {
    int level = 1;
    while ((double)rand() / RAND_MAX < sl->p && level < sl->max_level) {
        level++;
    }
    return level;
}

static ZSkipListNode* skiplist_create_node(int level, void* member, double score) {
    ZSkipListNode* node = malloc(sizeof(*node) + level * sizeof(ZSkipListLevel));
    if (node == NULL) {
        return NULL;
    }
    node->member = member;
    node->score = score;
    node->backward = NULL;
    for (int i = 0; i < level; ++i) {
        node->level[i].forward = NULL;
        node->level[i].span = 0;
    }
    return node;
}

ZSkipList* skiplist_create(int max_level,
                           double p,
                           ZSetCompareFn compare,
                           size_t member_size) {
    if (max_level < 1 || max_level > 64 || p <= 0.0 || p >= 1.0 || compare == NULL) {
        return NULL;
    }
    ZSkipList* sl = calloc(1, sizeof(*sl));
    if (sl == NULL) {
        return NULL;
    }
    sl->header = skiplist_create_node(max_level, NULL, 0.0);
    if (sl->header == NULL) {
        free(sl);
        return NULL;
    }
    sl->level = 1;
    sl->max_level = max_level;
    sl->p = p;
    sl->length = 0;
    sl->tail = NULL;
    sl->compare = compare;
    sl->member_size = member_size;
    srand((unsigned int)time(NULL));
    return sl;
}

void skiplist_destroy(ZSkipList* sl,
                      void (*free_member)(void* member)) {
    if (sl == NULL) {
        return;
    }
    ZSkipListNode* node = sl->header->level[0].forward;
    while (node != NULL) {
        ZSkipListNode* next = node->level[0].forward;
        if (free_member != NULL && node->member != NULL) {
            free_member(node->member);
        }
        free(node);
        node = next;
    }
    free(sl->header);
    free(sl);
}

ZSkipListNode* skiplist_search(ZSkipList* sl,
                               const void* member,
                               double score) {
    if (sl == NULL || member == NULL) {
        return NULL;
    }
    ZSkipListNode* node = sl->header;
    for (int i = sl->level - 1; i >= 0; --i) {
        while (node->level[i].forward != NULL) {
            ZSkipListNode* next = node->level[i].forward;
            if (next->score > score || (next->score == score && sl->compare(next->member, member, sl->member_size) >= 0)) {
                break;
            }
            node = next;
        }
    }
    node = node->level[0].forward;
    if (node != NULL && node->score == score && sl->compare(node->member, member, sl->member_size) == 0) {
        return node;
    }
    return NULL;
}

ZSkipListNode* skiplist_get_element_by_rank(ZSkipList* sl,
                                            unsigned long rank) {
    if (sl == NULL || rank >= sl->length) {
        return NULL;
    }
    ZSkipListNode* node = sl->header;
    unsigned long traversed = 0;
    for (int i = sl->level - 1; i >= 0; --i) {
        while (node->level[i].forward != NULL && traversed + node->level[i].span <= rank) {
            traversed += node->level[i].span;
            node = node->level[i].forward;
        }
    }
    return node->level[0].forward;
}

unsigned long skiplist_get_rank(ZSkipList* sl,
                                const void* member,
                                double score) {
    if (sl == NULL || member == NULL) {
        return 0;
    }
    ZSkipListNode* node = sl->header;
    unsigned long rank = 0;
    for (int i = sl->level - 1; i >= 0; --i) {
        while (node->level[i].forward != NULL) {
            ZSkipListNode* next = node->level[i].forward;
            if (next->score > score || (next->score == score && sl->compare(next->member, member, sl->member_size) > 0)) {
                break;
            }
            rank += node->level[i].span;
            node = next;
        }
        if (node->score == score && sl->compare(node->member, member, sl->member_size) == 0) {
            return rank;
        }
    }
    node = node->level[0].forward;
    if (node != NULL && node->score == score && sl->compare(node->member, member, sl->member_size) == 0) {
        return rank;
    }
    return 0;
}

static bool skiplist_node_matches(ZSkipListNode* node,
                                  const void* member,
                                  double score,
                                  ZSkipList* sl) {
    return node != NULL && node->score == score && sl->compare(node->member, member, sl->member_size) == 0;
}

static ZSkipListNode* skiplist_find_update(ZSkipList* sl,
                                           const void* member,
                                           double score,
                                           ZSkipListNode** update,
                                           unsigned long* rank) {
    ZSkipListNode* node = sl->header;
    unsigned long traversed = 0;
    for (int i = sl->level - 1; i >= 0; --i) {
        while (node->level[i].forward != NULL) {
            ZSkipListNode* next = node->level[i].forward;
            if (next->score > score || (next->score == score && sl->compare(next->member, member, sl->member_size) >= 0)) {
                break;
            }
            traversed += node->level[i].span;
            node = next;
        }
        update[i] = node;
        rank[i] = traversed;
    }
    return node;
}

ZSkipListNode* skiplist_insert(ZSkipList* sl,
                               void* member,
                               double score) {
    if (sl == NULL || member == NULL) {
        return NULL;
    }
    ZSkipListNode* update[64];
    unsigned long rank[64];
    skiplist_find_update(sl, member, score, update, rank);
    int level = skiplist_random_level(sl);
    if (level > sl->level) {
        for (int i = sl->level; i < level; ++i) {
            rank[i] = 0;
            update[i] = sl->header;
            update[i]->level[i].span = sl->length;
        }
        sl->level = level;
    }
    ZSkipListNode* new_node = skiplist_create_node(level, member, score);
    if (new_node == NULL) {
        return NULL;
    }
    for (int i = 0; i < level; ++i) {
        new_node->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = new_node;
        new_node->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);
        update[i]->level[i].span = (unsigned int)(rank[0] - rank[i] + 1);
    }
    for (int i = level; i < sl->level; ++i) {
        update[i]->level[i].span += 1;
    }
    new_node->backward = (update[0] == sl->header) ? NULL : update[0];
    if (new_node->level[0].forward != NULL) {
        new_node->level[0].forward->backward = new_node;
    } else {
        sl->tail = new_node;
    }
    sl->length += 1;
    return new_node;
}

bool skiplist_delete(ZSkipList* sl,
                     const void* member,
                     double score) {
    if (sl == NULL || member == NULL) {
        return false;
    }
    ZSkipListNode* update[64];
    unsigned long rank[64];
    ZSkipListNode* node = skiplist_find_update(sl, member, score, update, rank)->level[0].forward;
    if (!skiplist_node_matches(node, member, score, sl)) {
        return false;
    }
    for (int i = 0; i < sl->level; ++i) {
        if (update[i]->level[i].forward == node) {
            update[i]->level[i].span += node->level[i].span - 1;
            update[i]->level[i].forward = node->level[i].forward;
        } else {
            update[i]->level[i].span -= 1;
        }
    }
    if (node->level[0].forward != NULL) {
        node->level[0].forward->backward = node->backward;
    } else {
        sl->tail = node->backward;
    }
    while (sl->level > 1 && sl->header->level[sl->level - 1].forward == NULL) {
        sl->level -= 1;
    }
    free(node);
    sl->length -= 1;
    return true;
}

bool skiplist_update_score(ZSkipList* sl,
                           ZSkipListNode* node,
                           double new_score) {
    if (sl == NULL || node == NULL) {
        return false;
    }
    void* member = node->member;
    double old_score = node->score;
    if (!skiplist_delete(sl, member, old_score)) {
        return false;
    }
    ZSkipListNode* inserted = skiplist_insert(sl, member, new_score);
    if (inserted == NULL) {
        return false;
    }
    return true;
}

ZSkipListNode* skiplist_first_in_score_range(ZSkipList* sl,
                                             double min,
                                             double max) {
    if (sl == NULL) {
        return NULL;
    }
    ZSkipListNode* node = sl->header;
    for (int i = sl->level - 1; i >= 0; --i) {
        while (node->level[i].forward != NULL && node->level[i].forward->score < min) {
            node = node->level[i].forward;
        }
    }
    node = node->level[0].forward;
    if (node == NULL || node->score > max) {
        return NULL;
    }
    return node;
}

ZSkipListNode* skiplist_last_in_score_range(ZSkipList* sl,
                                            double min,
                                            double max) {
    if (sl == NULL) {
        return NULL;
    }
    ZSkipListNode* node = sl->header;
    for (int i = sl->level - 1; i >= 0; --i) {
        while (node->level[i].forward != NULL && node->level[i].forward->score <= max) {
            node = node->level[i].forward;
        }
    }
    if (node == sl->header || node->score < min || node->score > max) {
        return NULL;
    }
    return node;
}
