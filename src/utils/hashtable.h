/**
 * @file hashtable.h
 * @brief Lightweight generic hash table for ZSet internal indexing.
 *
 * This implementation uses separate chaining, dynamic resizing, and
 * customizable hash and key equality callbacks.
 */

#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Hash callback used by HashTable.
 * @param key pointer to the entry key
 * @param key_size size of key bytes or zero for callback-managed keys
 * @return 64-bit hash value
 */
typedef uint64_t (*HashFn)(const void* key, size_t key_size);

/**
 * @brief Key comparison callback used by HashTable.
 * @return zero if equal, negative if a < b, positive if a > b
 */
typedef int (*KeyCompareFn)(const void* a, const void* b, size_t key_size);

/**
 * @brief Hash table entry storing a key-value pair.
 */
typedef struct HashEntry {
    void* key;
    void* value;
    struct HashEntry* next;
} HashEntry;

/**
 * @brief Hash table structure.
 */
typedef struct HashTable {
    HashEntry** buckets;
    size_t size;
    size_t used;
    size_t key_size;
    HashFn hash;
    KeyCompareFn compare;
} HashTable;

HashTable* hash_create(size_t initial_size,
                       size_t key_size,
                       HashFn hash,
                       KeyCompareFn compare);
void hash_destroy(HashTable* ht);
void* hash_put(HashTable* ht, void* key, void* value);
void* hash_get(const HashTable* ht, const void* key);
void* hash_remove(HashTable* ht, const void* key);
bool hash_resize(HashTable* ht, size_t new_size);
uint64_t hash_default(const void* key, size_t key_size);
int compare_default(const void* a, const void* b, size_t key_size);

#ifdef __cplusplus
}
#endif

#endif /* HASHTABLE_H */
