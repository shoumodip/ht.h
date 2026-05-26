// MIT License
//
// Copyright (c) 2026 Shoumodip Kar
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef HT_H
#define HT_H

// Example:
// ```
// #include <stdio.h>
//
// #define HT_IMPLEMENTATION
// #include "ht.h"
//
// int main(void) {
//     HT(const char *, int) ht = {.hasheq = ht_hasheq_cstr};
//
//     ht_set(&ht, "foo", 69);
//     ht_set(&ht, "bar", 420);
//     ht_set(&ht, "baz", 1337);
//
//     ht_foreach(my, &ht) {
//         printf("%s => %d\n", *my.key, *my.value);
//     }
//
//     ht_delete(&ht, "foo");
//     printf("\nfoo => %p\n", ht_get(&ht, "foo"));
//
//     ht_free(&ht);
//     return 0;
// }
// ```

// Public API ////////////////////////////////////////////////////////////////////////////////
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define HT(K, V)                                                                                                       \
    struct {                                                                                                           \
        struct {                                                                                                       \
            uint8_t state;                                                                                             \
            K       key;                                                                                               \
            V       value;                                                                                             \
        } *data;                                                                                                       \
                                                                                                                       \
        size_t    count;                                                                                               \
        size_t    capacity;                                                                                            \
        HT_Hasheq hasheq;                                                                                              \
    }

// For providing custom hashing and equality functionality. The contract is:
//
// ```
// hasheq(&key, NULL, sizeof(key)) => Hash 'key'
// hasheq(&a, &b, sizeof(a))       => Compare 'a' and 'b' and return non-zero if equal
// ```
//
// By default, the bytes of the key are compared. For having C-strings as the key, the function 'ht_hasheq_cstr' is
// already provided.
// ```
// HT(const char *, int) ht = {.hasheq = ht_hasheq_cstr};
// ```
typedef uint64_t (*HT_Hasheq)(const void *a, const void *b, size_t n);

// V *ht_get(HT(K, V) *ht, K key)
#define ht_get(ht, k)                                                                                                  \
    ((__typeof__((ht)->data->value) *) ht_get_impl(                                                                    \
        (ht)->data, (ht)->capacity, HT_LAYOUT(ht), (ht)->hasheq, (__typeof__((ht)->data->key)[]) {k}))

// void ht_get(HT(K, V) *ht, K key, V value)
#define ht_set(ht, k, v)                                                                                               \
    (ht_set_impl(                                                                                                      \
        (void **) &(ht)->data,                                                                                         \
        &(ht)->count,                                                                                                  \
        &(ht)->capacity,                                                                                               \
        HT_LAYOUT(ht),                                                                                                 \
        (ht)->hasheq,                                                                                                  \
        (__typeof__((ht)->data->key)[]) {k},                                                                           \
        (__typeof__((ht)->data->value)[]) {v}))

// void ht_delete(HT(K, V) *ht, K key)
#define ht_delete(ht, k)                                                                                               \
    (ht_delete_impl(                                                                                                   \
        (ht)->data, &(ht)->count, (ht)->capacity, HT_LAYOUT(ht), (ht)->hasheq, (__typeof__((ht)->data->key)[]) {k}))

// void ht_clear(HT(K, V) *ht)
#define ht_clear(ht) ((ht)->data ? memset((ht)->data, 0, (ht)->capacity * sizeof(*(ht)->data)) : NULL, (ht)->count = 0)

// void ht_free(HT(K, V) *ht)
#define ht_free(ht) (free((ht)->data), memset((ht), 0, sizeof(*(ht))))

// HT(const char *, int) xs = {0};
// ...
// ht_foreach(x, &xs) {
//     printf("%s => %d\n", *x.key, *x.value);
// }
#define ht_foreach(it, ht)                                                                                             \
    for (HT_Iter(__typeof__((ht)->data->key), __typeof__((ht)->data->value)) it = {0}; ht_iter(ht, it);)

#define HT_Iter(K, V)                                                                                                  \
    struct {                                                                                                           \
        const K *key;                                                                                                  \
        V       *value;                                                                                                \
                                                                                                                       \
        bool   started;                                                                                                \
        size_t index;                                                                                                  \
    }

// bool ht_iter(HT(K, V) *ht, HT_Iter(K, V) *it)
#define ht_iter(ht, it)                                                                                                \
    ht_iter_impl(                                                                                                      \
        (ht)->data,                                                                                                    \
        (ht)->capacity,                                                                                                \
        HT_LAYOUT(ht),                                                                                                 \
        &(it.started),                                                                                                 \
        &(it).index,                                                                                                   \
        (void *) &(it).key,                                                                                            \
        (void *) &(it).value)

// Private Definitions ////////////////////////////////////////////////////////////////////////////////
#define HT_EMPTY     0
#define HT_OCCUPIED  1
#define HT_TOMBSTONE 2

#define HT_LOAD     0.75
#define HT_INIT_CAP 16

typedef struct {
    size_t key_offset;
    size_t key_size;

    size_t value_offset;
    size_t value_size;

    size_t entry_size;
} HT_Layout;

#define HT_LAYOUT(ht)                                                                                                  \
    ((HT_Layout) {                                                                                                     \
        .key_offset = offsetof(__typeof__(*(ht)->data), key),                                                          \
        .key_size = sizeof((ht)->data->key),                                                                           \
                                                                                                                       \
        .value_offset = offsetof(__typeof__(*(ht)->data), value),                                                      \
        .value_size = sizeof((ht)->data->value),                                                                       \
                                                                                                                       \
        .entry_size = sizeof(*(ht)->data),                                                                             \
    })

uint64_t ht_hasheq_bytes(const void *a, const void *b, size_t n);
uint64_t ht_hasheq_cstr(const void *a, const void *b, size_t n);

void *ht_find_impl(void *data, size_t capacity, HT_Layout layout, HT_Hasheq hasheq, const void *key);
void *ht_get_impl(void *data, size_t capacity, HT_Layout layout, HT_Hasheq hasheq, const void *key);
void  ht_set_impl(
     void      **ht_data,
     size_t     *ht_count,
     size_t     *ht_capacity,
     HT_Layout   layout,
     HT_Hasheq   hasheq,
     const void *key,
     const void *value);
void ht_delete_impl(void *data, size_t *count, size_t capacity, HT_Layout layout, HT_Hasheq hasheq, const void *key);

bool ht_iter_impl(
    void *data, size_t capacity, HT_Layout layout, bool *started, size_t *index, void **key, void **value);

#endif // HT_H

#ifdef HT_IMPLEMENTATION

#include <assert.h>

uint64_t ht_hasheq_bytes(const void *a, const void *b, size_t n) {
    if (b) {
        return memcmp(a, b, n) == 0;
    }

    uint64_t hash = 14695981039346656037UL;
    for (size_t i = 0; i < n; i++) {
        hash ^= ((uint8_t *) a)[i];
        hash *= 1099511628211UL;
    }
    return hash;
}

uint64_t ht_hasheq_cstr(const void *va, const void *vb, size_t n) {
    const char *a = *(const char **) va;
    if (vb) {
        return strcmp(a, *(const char **) vb) == 0;
    }

    uint64_t hash = 14695981039346656037UL;
    for (const char *p = a; *p; p++) {
        hash ^= *p;
        hash *= 1099511628211UL;
    }
    return hash;
}

void *ht_find_impl(void *data, size_t capacity, HT_Layout layout, HT_Hasheq hasheq, const void *key) {
    if (!hasheq) {
        hasheq = ht_hasheq_bytes;
    }

    const uint64_t start = hasheq(key, NULL, layout.key_size);
    void          *tombstone = NULL;
    for (size_t i = 0; i < capacity; i++) {
        const size_t index = (start + i) & (capacity - 1);
        uint8_t     *entry = (uint8_t *) data + index * layout.entry_size;

        switch (*entry) {
        case HT_EMPTY:
            return tombstone ? tombstone : entry;

        case HT_OCCUPIED:
            if (hasheq(key, entry + layout.key_offset, layout.key_size)) {
                return entry;
            }
            break;

        case HT_TOMBSTONE:
            if (!tombstone) {
                tombstone = entry;
            }
            break;
        }
    }
    return tombstone;
}

void *ht_get_impl(void *data, size_t capacity, HT_Layout layout, HT_Hasheq hasheq, const void *key) {
    uint8_t *entry = (uint8_t *) ht_find_impl(data, capacity, layout, hasheq, key);
    return (entry && *entry == HT_OCCUPIED) ? entry + layout.value_offset : NULL;
}

void ht_set_impl(
    void      **ht_data,
    size_t     *ht_count,
    size_t     *ht_capacity,
    HT_Layout   layout,
    HT_Hasheq   hasheq,
    const void *key,
    const void *value) //
{
    if (*ht_count >= *ht_capacity * HT_LOAD) {
        const size_t capacity = *ht_capacity ? *ht_capacity * 2 : HT_INIT_CAP;
        void        *data = calloc(capacity, layout.entry_size);
        for (size_t i = 0; i < *ht_capacity; i++) {
            uint8_t *src = (uint8_t *) *ht_data + i * layout.entry_size;
            if (*src == HT_OCCUPIED) {
                uint8_t *dst = (uint8_t *) ht_find_impl(data, capacity, layout, hasheq, src + layout.key_offset);
                assert(dst);

                *dst = HT_OCCUPIED;
                memcpy(dst + layout.key_offset, src + layout.key_offset, layout.key_size);
                memcpy(dst + layout.value_offset, src + layout.value_offset, layout.value_size);
            }
        }

        free(*ht_data);
        *ht_data = data;
        *ht_capacity = capacity;
    }

    uint8_t *dst = (uint8_t *) ht_find_impl(*ht_data, *ht_capacity, layout, hasheq, key);
    assert(dst);

    if (*dst != HT_OCCUPIED) {
        *dst = HT_OCCUPIED;
        memcpy(dst + layout.key_offset, key, layout.key_size);
        (*ht_count)++;
    }
    memcpy(dst + layout.value_offset, value, layout.value_size);
}

void ht_delete_impl(void *data, size_t *count, size_t capacity, HT_Layout layout, HT_Hasheq hasheq, const void *key) {
    uint8_t *entry = (uint8_t *) ht_find_impl(data, capacity, layout, hasheq, key);
    if (entry && *entry == HT_OCCUPIED) {
        *entry = HT_TOMBSTONE;
        (*count)--;
    }
}

bool ht_iter_impl(
    void *data, size_t capacity, HT_Layout layout, bool *started, size_t *index, void **key, void **value) {
    if (*started) {
        (*index)++;
    } else {
        *started = true;
    }

    for (size_t i = *index; i < capacity; i++) {
        uint8_t *entry = (uint8_t *) data + i * layout.entry_size;
        if (*entry == HT_OCCUPIED) {
            *index = i;
            *key = entry + layout.key_offset;
            *value = entry + layout.value_offset;
            return true;
        }
    }

    return false;
}

#endif // HT_IMPLEMENTATION
