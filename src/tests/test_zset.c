/**
 * @file test_zset.c
 * @brief Unit tests for the Redis-like ZSet sorted set implementation.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../utils/zset.h"

static void print_entry(const void* member, double score, void* user_data) {
    (void)user_data;
    const char* text = *(const char* const*)member;
    printf("%s: %.2f\n", text, score);
}

static void print_int_entry(const void* member, double score, void* user_data) {
    (void)user_data;
    printf("%d => %.2f\n", *(const int*)member, score);
}

static void test_string_zset(void) {
    ZSet* zs = zset_create(sizeof(char*), zset_string_hash, zset_string_compare, zset_string_dup, zset_string_free);
    assert(zs != NULL);

    const char* items[] = {"alpha", "beta", "gamma", "delta", "epsilon"};
    double scores[] = {9.5, 3.0, 7.2, 1.1, 7.2};

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); ++i) {
        assert(zset_add(zs, &items[i], scores[i]) == ZSET_OK);
    }
    assert(zset_size(zs) == 5);

    size_t rank = 0;
    assert(zset_rank(zs, &items[0], &rank) == ZSET_OK);

    double score = 0.0;
    assert(zset_score(zs, &items[1], &score) == ZSET_OK && score == 3.0);
    assert(zset_exists(zs, &items[2]));

    assert(zset_update_score(zs, &items[3], 8.8) == ZSET_OK);
    assert(zset_increment_score(zs, &items[3], 1.2, &score) == ZSET_OK && score == 10.0);

    printf("-- range by score [3.0, 8.0] --\n");
    assert(zset_range_by_score(zs, 3.0, 8.0, print_entry, NULL) == ZSET_OK);

    printf("-- reverse range by score [1.0, 10.0] --\n");
    assert(zset_rev_range_by_score(zs, 1.0, 10.0, print_entry, NULL) == ZSET_OK);

    void* top_member = NULL;
    double top_score = 0.0;
    assert(zset_top(zs, &top_member, &top_score) == ZSET_OK);
    assert(top_member != NULL);
    zset_string_free(top_member);

    void* min_member = NULL;
    double min_score = 0.0;
    assert(zset_bottom(zs, &min_member, &min_score) == ZSET_OK);
    assert(min_member != NULL);
    zset_string_free(min_member);

    size_t removed = 0;
    assert(zset_remove_by_score(zs, 7.0, 10.0, &removed) == ZSET_OK);
    assert(removed >= 1);

    assert(zset_remove(zs, &items[1]) == ZSET_OK);
    assert(!zset_exists(zs, &items[1]));

    zset_destroy(&zs);
    assert(zs == NULL);
}

static void test_int_zset(void) {
    ZSet* zs = zset_create(sizeof(int), zset_int_hash, zset_int_compare, NULL, NULL);
    assert(zs != NULL);

    int values[] = {10, 5, 15, 5, 20};
    double scores[] = {1.0, 2.0, 3.0, 2.0, 4.0};
    for (size_t i = 0; i < 5; ++i) {
        if (i == 3) {
            assert(zset_add(zs, &values[i], scores[i]) == ZSET_ERR_DUPLICATE);
            continue;
        }
        assert(zset_add(zs, &values[i], scores[i]) == ZSET_OK);
    }
    assert(zset_size(zs) == 4);

    printf("-- ordered int zset --\n");
    assert(zset_range(zs, 0, zset_size(zs) - 1, print_int_entry, NULL) == ZSET_OK);

    size_t rank = 0;
    int key = 15;
    assert(zset_rank(zs, &key, &rank) == ZSET_OK);
    assert(rank == 3);

    int remove_key = 10;
    assert(zset_remove(zs, &remove_key) == ZSET_OK);
    assert(!zset_exists(zs, &remove_key));

    zset_destroy(&zs);
}

int main(void) {
    printf("=== ZSet unit tests ===\n");
    test_string_zset();
    test_int_zset();
    printf("All ZSet tests passed.\n");
    return 0;
}
