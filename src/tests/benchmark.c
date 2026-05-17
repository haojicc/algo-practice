/**
 * @file benchmark.c
 * @brief Benchmark suite for ZSet operations.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#include "../utils/zset.h"

static double elapsed_seconds(struct timespec start, struct timespec end) {
    return (double)(end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

static void benchmark_zset(size_t count) {
    printf("\n=== benchmark %zu elements ===\n", count);
    ZSet* zs = zset_create(sizeof(int), zset_int_hash, zset_int_compare, NULL, NULL);
    if (zs == NULL) {
        fprintf(stderr, "Failed to create ZSet\n");
        exit(EXIT_FAILURE);
    }

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (size_t i = 0; i < count; ++i) {
        int key = (int)i;
        zset_add(zs, &key, (double)(count - i));
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double insert_time = elapsed_seconds(start, end);
    printf("insert: %.3fs, qps: %.0f\n", insert_time, count / insert_time);

    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t found = 0;
    for (size_t i = 0; i < count; i += count / 10 + 1) {
        int key = (int)i;
        double score;
        if (zset_score(zs, &key, &score) == ZSET_OK) {
            found += 1;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double query_time = elapsed_seconds(start, end);
    printf("lookup %zu keys: %.3fs, qps: %.0f\n", found, query_time, found / query_time);

    clock_gettime(CLOCK_MONOTONIC, &start);
    size_t removed = 0;
    size_t limit = count / 10;
    for (size_t i = 0; i < limit; ++i) {
        size_t start_rank = i * 10;
        size_t end_rank = start_rank + 5;
        zset_remove_by_rank(zs, start_rank, end_rank, &removed);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("remove by rank: %.3fs\n", elapsed_seconds(start, end));

    zset_destroy(&zs);
}

int main(void) {
    benchmark_zset(100000);
    benchmark_zset(1000000);
    benchmark_zset(10000000);
    return 0;
}
