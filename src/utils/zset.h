/**
 * @file zset.h
 * @brief Redis-like sorted set (ZSet) API using skip list + hash table.
 *
 * This header exposes a production-style sorted set implementation with
 * O(log N) insert, delete, rank and range query operations.
 */

#ifndef ZSET_H
#define ZSET_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Result codes returned by ZSet operations.
 */
typedef enum ZSetResult {
    ZSET_OK = 0,
    ZSET_ERR_ALLOC,
    ZSET_ERR_NOT_FOUND,
    ZSET_ERR_DUPLICATE,
    ZSET_ERR_INVALID
} ZSetResult;

/**
 * @brief Callback for hashing a member.
 */
typedef uint64_t (*ZSetHashFn)(const void* member, size_t member_size);

/**
 * @brief Callback for comparing two members.
 */
typedef int (*ZSetMemberCompareFn)(const void* a,
                                   const void* b,
                                   size_t member_size);

/**
 * @brief Callback for cloning a member into internal storage.
 */
typedef void* (*ZSetMemberDupFn)(const void* member,
                                  size_t member_size);

/**
 * @brief Callback for freeing a member.
 */
typedef void (*ZSetMemberFreeFn)(void* member);

/**
 * @brief Visit callback for iteration and range queries.
 */
typedef void (*ZSetVisitFn)(const void* member,
                            double score,
                            void* user_data);

/**
 * @brief Iterator for scanning a ZSet.
 */
typedef struct ZSetIterator {
    const struct ZSet* zset;
    void* current_member;
    double current_score;
    bool reverse;
    void* current_node;
} ZSetIterator;

typedef struct ZSet ZSet;

ZSet* zset_create(size_t member_size,
                  ZSetHashFn hash_fn,
                  ZSetMemberCompareFn compare_fn,
                  ZSetMemberDupFn dup_fn,
                  ZSetMemberFreeFn free_fn);

void zset_destroy(ZSet** zsp);
ZSetResult zset_clear(ZSet* zs);

ZSetResult zset_add(ZSet* zs, const void* member, double score);
ZSetResult zset_update_score(ZSet* zs, const void* member, double score);
ZSetResult zset_increment_score(ZSet* zs, const void* member, double delta, double* out_score);

ZSetResult zset_remove(ZSet* zs, const void* member);
ZSetResult zset_remove_by_score(ZSet* zs,
                                double min,
                                double max,
                                size_t* removed);
ZSetResult zset_remove_by_rank(ZSet* zs,
                               size_t start,
                               size_t end,
                               size_t* removed);

ZSetResult zset_score(const ZSet* zs, const void* member, double* score);
ZSetResult zset_rank(const ZSet* zs, const void* member, size_t* rank);
ZSetResult zset_rev_rank(const ZSet* zs, const void* member, size_t* rank);
bool zset_exists(const ZSet* zs, const void* member);
size_t zset_size(const ZSet* zs);

ZSetResult zset_range(const ZSet* zs,
                      size_t start,
                      size_t end,
                      ZSetVisitFn visit,
                      void* user_data);
ZSetResult zset_rev_range(const ZSet* zs,
                          size_t start,
                          size_t end,
                          ZSetVisitFn visit,
                          void* user_data);
ZSetResult zset_range_by_score(const ZSet* zs,
                               double min,
                               double max,
                               ZSetVisitFn visit,
                               void* user_data);
ZSetResult zset_rev_range_by_score(const ZSet* zs,
                                   double min,
                                   double max,
                                   ZSetVisitFn visit,
                                   void* user_data);

ZSetResult zset_top(const ZSet* zs, void** out_member, double* out_score);
ZSetResult zset_bottom(const ZSet* zs, void** out_member, double* out_score);
ZSetResult zset_pop_max(ZSet* zs, void** out_member, double* out_score);
ZSetResult zset_pop_min(ZSet* zs, void** out_member, double* out_score);

ZSetResult zset_foreach(const ZSet* zs,
                        ZSetVisitFn visit,
                        void* user_data);

void zset_iterator_init(ZSetIterator* it,
                        const ZSet* zs,
                        bool reverse);
bool zset_iterator_next(ZSetIterator* it,
                        void** out_member,
                        double* out_score);

/* Built-in helpers for common member types */
uint64_t zset_default_hash(const void* member, size_t member_size);
int zset_default_compare(const void* a, const void* b, size_t member_size);
uint64_t zset_string_hash(const void* member, size_t member_size);
int zset_string_compare(const void* a, const void* b, size_t member_size);
void* zset_string_dup(const void* member, size_t member_size);
void zset_string_free(void* member);
uint64_t zset_int_hash(const void* member, size_t member_size);
int zset_int_compare(const void* a, const void* b, size_t member_size);

#ifdef __cplusplus
}
#endif

#endif /* ZSET_H */
