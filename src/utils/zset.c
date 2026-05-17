/**
 * @file zset.c
 * @brief Redis-like sorted set implementation using skip list + hash table.
 */

#include "zset.h"
#include "hashtable.h"
#include "skiplist.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct ZSetEntry {
    void* member;
    double score;
};

struct ZSet {
    size_t member_size;
    HashTable* dict;
    ZSkipList* sl;
    ZSetHashFn hash_fn;
    ZSetMemberCompareFn compare_fn;
    ZSetMemberDupFn dup_fn;
    ZSetMemberFreeFn free_fn;
};

static void* zset_member_dup(const ZSet* zs, const void* member) {
    if (zs == NULL || member == NULL) {
        return NULL;
    }
    if (zs->dup_fn != NULL) {
        return zs->dup_fn(member, zs->member_size);
    }
    void* copy = malloc(zs->member_size);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, member, zs->member_size);
    return copy;
}

static void zset_member_free_internal(const ZSet* zs, void* member) {
    if (zs == NULL || member == NULL) {
        return;
    }
    if (zs->free_fn != NULL) {
        zs->free_fn(member);
    } else {
        free(member);
    }
}

uint64_t zset_default_hash(const void* member, size_t member_size) {
    if (member == NULL) {
        return 0;
    }
    const unsigned char* bytes = (const unsigned char*)member;
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < member_size; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

int zset_default_compare(const void* a, const void* b, size_t member_size) {
    if (a == b) {
        return 0;
    }
    int result = memcmp(a, b, member_size);
    if (result < 0) {
        return -1;
    }
    if (result > 0) {
        return 1;
    }
    return 0;
}

uint64_t zset_string_hash(const void* member, size_t member_size) {
    (void)member_size;
    if (member == NULL) {
        return 0;
    }
    const char* text = *(const char* const*)member;
    uint64_t hash = 1469598103934665603ULL;
    while (*text != '\0') {
        hash ^= (uint64_t)(unsigned char)*text++;
        hash *= 1099511628211ULL;
    }
    return hash;
}

int zset_string_compare(const void* a, const void* b, size_t member_size) {
    (void)member_size;
    const char* left = *(const char* const*)a;
    const char* right = *(const char* const*)b;
    return strcmp(left, right);
}

void* zset_string_dup(const void* member, size_t member_size) {
    (void)member_size;
    if (member == NULL) {
        return NULL;
    }
    const char* source = *(const char* const*)member;
    if (source == NULL) {
        return NULL;
    }
    char* copy = strdup(source);
    if (copy == NULL) {
        return NULL;
    }
    char** container = malloc(sizeof(char*));
    if (container == NULL) {
        free(copy);
        return NULL;
    }
    *container = copy;
    return container;
}

void zset_string_free(void* member) {
    if (member == NULL) {
        return;
    }
    char* text = *(char**)member;
    free(text);
    free(member);
}

uint64_t zset_int_hash(const void* member, size_t member_size) {
    (void)member_size;
    if (member == NULL) {
        return 0;
    }
    int value = *(const int*)member;
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < sizeof(value); ++i) {
        hash ^= (uint64_t)(((unsigned char*)&value)[i]);
        hash *= 1099511628211ULL;
    }
    return hash;
}

int zset_int_compare(const void* a, const void* b, size_t member_size) {
    (void)member_size;
    int lhs = *(const int*)a;
    int rhs = *(const int*)b;
    return (lhs < rhs) ? -1 : (lhs > rhs) ? 1 : 0;
}

static unsigned long zset_get_rank_internal(const ZSet* zs,
                                            const void* member,
                                            double score,
                                            bool reverse) {
    if (zs == NULL || member == NULL) {
        return 0;
    }
    unsigned long rank = skiplist_get_rank(zs->sl, member, score);
    if (reverse) {
        if (rank >= zs->sl->length) {
            return 0;
        }
        return zs->sl->length - rank - 1;
    }
    return rank;
}

ZSet* zset_create(size_t member_size,
                  ZSetHashFn hash_fn,
                  ZSetMemberCompareFn compare_fn,
                  ZSetMemberDupFn dup_fn,
                  ZSetMemberFreeFn free_fn) {
    if (member_size == 0 || compare_fn == NULL || hash_fn == NULL) {
        return NULL;
    }
    ZSet* zs = calloc(1, sizeof(*zs));
    if (zs == NULL) {
        return NULL;
    }
    zs->member_size = member_size;
    zs->hash_fn = hash_fn;
    zs->compare_fn = compare_fn;
    zs->dup_fn = dup_fn;
    zs->free_fn = free_fn;
    zs->dict = hash_create(64, member_size, hash_fn, compare_fn);
    if (zs->dict == NULL) {
        free(zs);
        return NULL;
    }
    zs->sl = skiplist_create(32, 0.25, compare_fn, member_size);
    if (zs->sl == NULL) {
        hash_destroy(zs->dict);
        free(zs);
        return NULL;
    }
    return zs;
}

void zset_destroy(ZSet** zsp) {
    if (zsp == NULL || *zsp == NULL) {
        return;
    }
    zset_clear(*zsp);
    hash_destroy((*zsp)->dict);
    skiplist_destroy((*zsp)->sl, NULL);
    free(*zsp);
    *zsp = NULL;
}

ZSetResult zset_clear(ZSet* zs) {
    if (zs == NULL) {
        return ZSET_ERR_INVALID;
    }
    ZSkipListNode* node = zs->sl->header->level[0].forward;
    while (node != NULL) {
        ZSkipListNode* next = node->level[0].forward;
        zset_member_free_internal(zs, node->member);
        node = next;
    }
    hash_destroy(zs->dict);
    zs->dict = hash_create(64, zs->member_size, zs->hash_fn, zs->compare_fn);
    if (zs->dict == NULL) {
        return ZSET_ERR_ALLOC;
    }
    skiplist_destroy(zs->sl, NULL);
    zs->sl = skiplist_create(32, 0.25, zs->compare_fn, zs->member_size);
    if (zs->sl == NULL) {
        return ZSET_ERR_ALLOC;
    }
    return ZSET_OK;
}

ZSetResult zset_add(ZSet* zs, const void* member, double score) {
    if (zs == NULL || member == NULL) {
        return ZSET_ERR_INVALID;
    }
    if (hash_get(zs->dict, member) != NULL) {
        return ZSET_ERR_DUPLICATE;
    }
    void* copy = zset_member_dup(zs, member);
    if (copy == NULL) {
        return ZSET_ERR_ALLOC;
    }
    ZSkipListNode* node = skiplist_insert(zs->sl, copy, score);
    if (node == NULL) {
        zset_member_free_internal(zs, copy);
        return ZSET_ERR_ALLOC;
    }
    struct ZSetEntry* entry = malloc(sizeof(*entry));
    if (entry == NULL) {
        skiplist_delete(zs->sl, copy, score);
        zset_member_free_internal(zs, copy);
        return ZSET_ERR_ALLOC;
    }
    entry->member = copy;
    entry->score = score;
    void* previous = hash_put(zs->dict, copy, entry);
    if (previous != NULL) {
        free(entry);
        skiplist_delete(zs->sl, copy, score);
        zset_member_free_internal(zs, copy);
        return ZSET_ERR_DUPLICATE;
    }
    return ZSET_OK;
}

ZSetResult zset_update_score(ZSet* zs, const void* member, double score) {
    if (zs == NULL || member == NULL) {
        return ZSET_ERR_INVALID;
    }
    struct ZSetEntry* entry = hash_get(zs->dict, member);
    if (entry == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    if (entry->score == score) {
        return ZSET_OK;
    }
    if (!skiplist_update_score(zs->sl, skiplist_search(zs->sl, entry->member, entry->score), score)) {
        return ZSET_ERR_INVALID;
    }
    entry->score = score;
    return ZSET_OK;
}

ZSetResult zset_increment_score(ZSet* zs, const void* member, double delta, double* out_score) {
    if (zs == NULL || member == NULL || out_score == NULL) {
        return ZSET_ERR_INVALID;
    }
    struct ZSetEntry* entry = hash_get(zs->dict, member);
    if (entry == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    double new_score = entry->score + delta;
    if (!skiplist_update_score(zs->sl, skiplist_search(zs->sl, entry->member, entry->score), new_score)) {
        return ZSET_ERR_INVALID;
    }
    entry->score = new_score;
    *out_score = new_score;
    return ZSET_OK;
}

ZSetResult zset_remove(ZSet* zs, const void* member) {
    if (zs == NULL || member == NULL) {
        return ZSET_ERR_INVALID;
    }
    struct ZSetEntry* entry = hash_remove(zs->dict, member);
    if (entry == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    if (!skiplist_delete(zs->sl, entry->member, entry->score)) {
        free(entry);
        return ZSET_ERR_INVALID;
    }
    zset_member_free_internal(zs, entry->member);
    free(entry);
    return ZSET_OK;
}

ZSetResult zset_remove_by_score(ZSet* zs,
                                double min,
                                double max,
                                size_t* removed) {
    if (zs == NULL || removed == NULL) {
        return ZSET_ERR_INVALID;
    }
    *removed = 0;
    ZSkipListNode* node = skiplist_first_in_score_range(zs->sl, min, max);
    while (node != NULL && node->score <= max) {
        ZSkipListNode* next = node->level[0].forward;
        void* member = node->member;
        double score = node->score;
        struct ZSetEntry* entry = hash_get(zs->dict, member);
        if (entry != NULL) {
            if (!skiplist_delete(zs->sl, member, score)) {
                node = next;
                continue;
            }
            hash_remove(zs->dict, member);
            zset_member_free_internal(zs, entry->member);
            free(entry);
            *removed += 1;
        }
        node = next;
    }
    return ZSET_OK;
}

ZSetResult zset_remove_by_rank(ZSet* zs,
                               size_t start,
                               size_t end,
                               size_t* removed) {
    if (zs == NULL || removed == NULL || start > end) {
        return ZSET_ERR_INVALID;
    }
    size_t size = zset_size(zs);
    if (start >= size) {
        *removed = 0;
        return ZSET_OK;
    }
    if (end >= size) {
        end = size - 1;
    }
    *removed = 0;
    ZSkipListNode* node = skiplist_get_element_by_rank(zs->sl, (unsigned long)start);
    while (node != NULL && *removed < end - start + 1) {
        ZSkipListNode* next = node->level[0].forward;
        void* member = node->member;
        double score = node->score;
        struct ZSetEntry* entry = hash_get(zs->dict, member);
        if (entry != NULL) {
            if (!skiplist_delete(zs->sl, member, score)) {
                node = next;
                continue;
            }
            hash_remove(zs->dict, member);
            zset_member_free_internal(zs, entry->member);
            free(entry);
            *removed += 1;
        }
        node = next;
    }
    return ZSET_OK;
}

ZSetResult zset_score(const ZSet* zs, const void* member, double* score) {
    if (zs == NULL || member == NULL || score == NULL) {
        return ZSET_ERR_INVALID;
    }
    struct ZSetEntry* entry = hash_get(zs->dict, member);
    if (entry == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    *score = entry->score;
    return ZSET_OK;
}

ZSetResult zset_rank(const ZSet* zs, const void* member, size_t* rank) {
    if (zs == NULL || member == NULL || rank == NULL) {
        return ZSET_ERR_INVALID;
    }
    struct ZSetEntry* entry = hash_get(zs->dict, member);
    if (entry == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    *rank = (size_t)zset_get_rank_internal(zs, member, entry->score, false);
    return ZSET_OK;
}

ZSetResult zset_rev_rank(const ZSet* zs, const void* member, size_t* rank) {
    if (zs == NULL || member == NULL || rank == NULL) {
        return ZSET_ERR_INVALID;
    }
    struct ZSetEntry* entry = hash_get(zs->dict, member);
    if (entry == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    *rank = (size_t)zset_get_rank_internal(zs, member, entry->score, true);
    return ZSET_OK;
}

bool zset_exists(const ZSet* zs, const void* member) {
    if (zs == NULL || member == NULL) {
        return false;
    }
    return hash_get(zs->dict, member) != NULL;
}

size_t zset_size(const ZSet* zs) {
    return zs != NULL ? zs->sl->length : 0;
}

ZSetResult zset_range(const ZSet* zs,
                      size_t start,
                      size_t end,
                      ZSetVisitFn visit,
                      void* user_data) {
    if (zs == NULL || visit == NULL || start > end) {
        return ZSET_ERR_INVALID;
    }
    size_t size = zset_size(zs);
    if (start >= size) {
        return ZSET_OK;
    }
    if (end >= size) {
        end = size - 1;
    }
    ZSkipListNode* node = skiplist_get_element_by_rank(zs->sl, (unsigned long)start);
    for (size_t index = start; index <= end && node != NULL; ++index) {
        visit(node->member, node->score, user_data);
        node = node->level[0].forward;
    }
    return ZSET_OK;
}

ZSetResult zset_rev_range(const ZSet* zs,
                          size_t start,
                          size_t end,
                          ZSetVisitFn visit,
                          void* user_data) {
    if (zs == NULL || visit == NULL || start > end) {
        return ZSET_ERR_INVALID;
    }
    size_t size = zset_size(zs);
    if (start >= size) {
        return ZSET_OK;
    }
    if (end >= size) {
        end = size - 1;
    }
    unsigned long index = size - 1 - start;
    ZSkipListNode* node = skiplist_get_element_by_rank(zs->sl, index);
    for (size_t count = start; count <= end && node != NULL; ++count) {
        visit(node->member, node->score, user_data);
        node = node->backward;
    }
    return ZSET_OK;
}

ZSetResult zset_range_by_score(const ZSet* zs,
                               double min,
                               double max,
                               ZSetVisitFn visit,
                               void* user_data) {
    if (zs == NULL || visit == NULL || min > max) {
        return ZSET_ERR_INVALID;
    }
    ZSkipListNode* node = skiplist_first_in_score_range(zs->sl, min, max);
    while (node != NULL && node->score <= max) {
        visit(node->member, node->score, user_data);
        node = node->level[0].forward;
    }
    return ZSET_OK;
}

ZSetResult zset_rev_range_by_score(const ZSet* zs,
                                   double min,
                                   double max,
                                   ZSetVisitFn visit,
                                   void* user_data) {
    if (zs == NULL || visit == NULL || min > max) {
        return ZSET_ERR_INVALID;
    }
    ZSkipListNode* node = skiplist_last_in_score_range(zs->sl, min, max);
    while (node != NULL && node->score >= min) {
        visit(node->member, node->score, user_data);
        node = node->backward;
    }
    return ZSET_OK;
}

static void* zset_copy_member(const ZSet* zs, const void* member) {
    if (zs == NULL || member == NULL) {
        return NULL;
    }
    if (zs->dup_fn != NULL) {
        return zs->dup_fn(member, zs->member_size);
    }
    void* copy = malloc(zs->member_size);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, member, zs->member_size);
    return copy;
}

ZSetResult zset_top(const ZSet* zs, void** out_member, double* out_score) {
    if (zs == NULL || out_member == NULL || out_score == NULL) {
        return ZSET_ERR_INVALID;
    }
    if (zs->sl->length == 0) {
        return ZSET_ERR_NOT_FOUND;
    }
    ZSkipListNode* node = zs->sl->header->level[0].forward;
    if (node == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    *out_score = node->score;
    *out_member = zset_copy_member(zs, node->member);
    return *out_member != NULL ? ZSET_OK : ZSET_ERR_ALLOC;
}

ZSetResult zset_bottom(const ZSet* zs, void** out_member, double* out_score) {
    if (zs == NULL || out_member == NULL || out_score == NULL) {
        return ZSET_ERR_INVALID;
    }
    if (zs->sl->length == 0) {
        return ZSET_ERR_NOT_FOUND;
    }
    ZSkipListNode* node = zs->sl->tail;
    if (node == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    *out_score = node->score;
    *out_member = zset_copy_member(zs, node->member);
    return *out_member != NULL ? ZSET_OK : ZSET_ERR_ALLOC;
}

ZSetResult zset_pop_min(ZSet* zs, void** out_member, double* out_score) {
    if (zs == NULL || out_member == NULL || out_score == NULL) {
        return ZSET_ERR_INVALID;
    }
    if (zs->sl->length == 0) {
        return ZSET_ERR_NOT_FOUND;
    }
    ZSkipListNode* node = zs->sl->header->level[0].forward;
    if (node == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    struct ZSetEntry* entry = hash_remove(zs->dict, node->member);
    if (entry == NULL) {
        return ZSET_ERR_INVALID;
    }
    *out_score = entry->score;
    *out_member = zset_copy_member(zs, entry->member);
    if (*out_member == NULL) {
        hash_put(zs->dict, entry->member, entry);
        return ZSET_ERR_ALLOC;
    }
    zset_member_free_internal(zs, entry->member);
    free(entry);
    if (!skiplist_delete(zs->sl, node->member, node->score)) {
        return ZSET_ERR_INVALID;
    }
    return ZSET_OK;
}

ZSetResult zset_pop_max(ZSet* zs, void** out_member, double* out_score) {
    if (zs == NULL || out_member == NULL || out_score == NULL) {
        return ZSET_ERR_INVALID;
    }
    if (zs->sl->length == 0) {
        return ZSET_ERR_NOT_FOUND;
    }
    ZSkipListNode* node = zs->sl->tail;
    if (node == NULL) {
        return ZSET_ERR_NOT_FOUND;
    }
    struct ZSetEntry* entry = hash_remove(zs->dict, node->member);
    if (entry == NULL) {
        return ZSET_ERR_INVALID;
    }
    *out_score = entry->score;
    *out_member = zset_copy_member(zs, entry->member);
    if (*out_member == NULL) {
        hash_put(zs->dict, entry->member, entry);
        return ZSET_ERR_ALLOC;
    }
    zset_member_free_internal(zs, entry->member);
    free(entry);
    if (!skiplist_delete(zs->sl, node->member, node->score)) {
        return ZSET_ERR_INVALID;
    }
    return ZSET_OK;
}

ZSetResult zset_foreach(const ZSet* zs,
                        ZSetVisitFn visit,
                        void* user_data) {
    if (zs == NULL || visit == NULL) {
        return ZSET_ERR_INVALID;
    }
    ZSkipListNode* node = zs->sl->header->level[0].forward;
    while (node != NULL) {
        visit(node->member, node->score, user_data);
        node = node->level[0].forward;
    }
    return ZSET_OK;
}

void zset_iterator_init(ZSetIterator* it,
                        const ZSet* zs,
                        bool reverse) {
    if (it == NULL || zs == NULL) {
        return;
    }
    it->zset = zs;
    it->reverse = reverse;
    if (reverse) {
        it->current_node = zs->sl->tail;
    } else {
        it->current_node = zs->sl->header->level[0].forward;
    }
}

bool zset_iterator_next(ZSetIterator* it,
                        void** out_member,
                        double* out_score) {
    if (it == NULL || out_member == NULL || out_score == NULL || it->current_node == NULL) {
        return false;
    }
    ZSkipListNode* node = (ZSkipListNode*)it->current_node;
    *out_member = node->member;
    *out_score = node->score;
    it->current_node = it->reverse ? node->backward : node->level[0].forward;
    return true;
}
