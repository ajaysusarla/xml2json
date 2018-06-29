/*
 * htable.h : A generic hash table implementation
 *
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */

#include "htable.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <limits.h>

#define FNV32_BASE  ((unsigned int) 0x811c9dc5)
#define FNV32_PRIME ((unsigned int) 0x01000193)

#define HTABLE_INIT_SIZE        64
#define HTABLE_RESIZE_BITS       2
#define HTABLE_RESIZE_THRESHOLD 80

/* Private functions */
static int default_cmp_fn(const void *data _unused_,
                          const void *entry1 _unused_,
                          const void *entry2 _unused_,
                          const void *kdata _unused_)
{
        return 0;
}

static void alloc_htable(struct htable *ht, size_t size)
{
        ht->size = size;
        ht->table = xcalloc(size, sizeof(struct htable *));

        ht->grow_mark = size * HTABLE_RESIZE_THRESHOLD / 100;
        if (size <= HTABLE_INIT_SIZE)
                ht->shrink_mark = 0;
        else
                ht->shrink_mark = ht->grow_mark / ((1 << HTABLE_RESIZE_BITS) + 1);
}

static inline int entries_equal(const struct htable *ht,
                                const struct htable_entry *e1,
                                const struct htable_entry *e2,
                                const void *kdata)
{
        return (e1 == e2) ||
                (e1->hash == e2->hash &&
                 !ht->cmpfn(ht->cmpfndata, e1, e2, kdata));
}

static inline unsigned int bucket(const struct htable *ht,
                                  const struct htable_entry *key)
{
        return key->hash & (ht->size - 1);
}

static inline struct htable_entry **find_entry(const struct htable *ht,
                                               const struct htable_entry *key,
        const void *keydata)
{
        struct htable_entry **e = &ht->table[bucket(ht, key)];

        while (*e && !entries_equal(ht, *e, key, keydata))
                e = &(*e)->next;

        return e;
}

static void rehash(struct htable *ht, unsigned int newsize)
{
        unsigned int i, oldsize = ht->size;
        struct htable_entry **oldtable = ht->table;

        alloc_htable(ht, newsize);
        for (i = 0; i < oldsize; i++) {
                struct htable_entry *e = oldtable[i];
                while (e) {
                        struct htable_entry *n = e->next;
                        unsigned int b = bucket(ht, e);
                        e->next = ht->table[b];
                        ht->table[b] = e;
                        e = n;
                }
        }

        free(oldtable);
}

/* Public functions */
unsigned int bufhash(const void *buf, size_t len)
{
        unsigned int hash = FNV32_BASE;
        const unsigned char *ptr = buf;

        while (len--) {
                unsigned int c = *ptr++;
                hash = (hash * FNV32_PRIME) ^ c;
        }

        return hash;
}

void htable_init(struct htable *ht, htable_cmp_fn cmp_fn,
                 const void *cmpfndata, size_t size)
{
        size_t init_size = HTABLE_INIT_SIZE;

        memset(ht, 0, sizeof(struct htable));

        ht->cmpfn = cmp_fn ? cmp_fn : default_cmp_fn;
        ht->cmpfndata = cmpfndata;
        ht->iter_head = ht->iter_tail = UINT_MAX;

        /* calculate initial size */
        size = size * 100 / HTABLE_RESIZE_THRESHOLD;
        while (size > init_size)
                init_size <<= HTABLE_RESIZE_BITS;

        alloc_htable(ht, init_size);
}

void htable_free(struct htable *ht, int free_entries)
{
        if (!ht || !ht->table)
                return;

        if (free_entries) {
                struct htable_iter iter;
                struct htable_entry *e;

                htable_iter_init(ht, &iter);
                while((e = htable_iter_next(&iter)))
                        free(e);
        }

        free(ht->table);
        memset(ht, 0, sizeof(struct htable));
}

void *htable_get(const struct htable *ht, const void *key, const void *keydata)
{
        return *find_entry(ht, key, keydata);
}

const void *htable_get_next(const struct htable *ht, const void *entry)
{
        const struct htable_entry *e = ((const struct htable_entry *) entry)->next;
        for (; e; e = e->next)
                if (entries_equal(ht, entry, e, NULL))
                        return e;

        return NULL;
}

void htable_put(struct htable *ht, void *entry)
{
        unsigned int b = bucket(ht, entry);
        int collision = 0;

        collision = ht->table[b] ? 1 : 0;

        if (collision) {
                ((struct htable_entry *) entry)->iter_prev =
                        ((struct htable_entry *) ht->table[b])->iter_prev;
                ((struct htable_entry *) entry)->iter_next =
                        ((struct htable_entry *) ht->table[b])->iter_next;
                ((struct htable_entry *) entry)->count =
                        ((struct htable_entry *) ht->table[b])->count;
        }

        ((struct htable_entry *) entry)->next = ht->table[b];
        ht->table[b] = entry;

        ((struct htable_entry *) entry)->count += 1;

        if (ht->iter_head == UINT_MAX) { /* First entry */
                ((struct htable_entry *) entry)->iter_prev = UINT_MAX;
                ((struct htable_entry *) entry)->iter_next = UINT_MAX;
                ht->iter_head = b;
                ht->iter_tail = b;
        } else {
                if (!collision) {
                        ((struct htable_entry *) ht->table[ht->iter_tail])->iter_next = b;
                        ((struct htable_entry *) entry)->iter_prev = ht->iter_tail;
                        ht->iter_tail = b;
                }
        }

        ht->count++;
        if (ht->count < ht->shrink_mark)
                rehash(ht, ht->size << HTABLE_RESIZE_BITS);
}

void *htable_remove(struct htable *ht, const void *key,
                    const void *keydata)
{
        struct htable_entry *old;
        struct htable_entry **e = find_entry(ht, key, keydata);

        if (!*e)
                return NULL;

        old = *e;
        *e = old->next;
        old->next = NULL;
        /* TODO: Update entry count */
        /* TODO: Update the iter_prev and iter_next values */

        ht->count--;
        if (ht->count < ht->shrink_mark)
                rehash(ht, ht->size >> HTABLE_RESIZE_BITS);

        return old;
}

void *htable_replace(struct htable *ht, void *entry)
{
        struct htable_entry *old = htable_remove(ht, entry, NULL);
        htable_put(ht, entry);

        return old;
}

void htable_iter_init(struct htable *ht, struct htable_iter *iter)
{
        iter->ht = ht;
        iter->pos = 0;
        iter->next = NULL;
}

void *htable_iter_next(struct htable_iter *iter)
{
        struct htable_entry *current = iter->next;

        for (;;){
                if (current) {
                        iter->next = current->next;
                        return current;
                }

                if (iter->pos >= iter->ht->size)
                        return NULL;

                current = iter->ht->table[iter->pos++];
        }
}

void htable_iter_init_ordered(struct htable *ht, struct htable_iter *iter)
{
        iter->ht = ht;
        iter->pos = ht->iter_head;
        iter->next = NULL;
}

void *htable_iter_ordered_get(struct htable_iter *iter)
{
        struct htable_entry *current = iter->ht->table[iter->pos];

        return current;
}

void *htable_iter_next_ordered(struct htable_iter *iter)
{
        struct htable_entry *current = NULL;

        /* if (iter->pos > iter->ht->count) */
        /*         return NULL; */

        if (iter->pos == UINT_MAX)
                return NULL;

        current = iter->ht->table[iter->pos];

        if (current) {
                iter->pos = current->iter_next;
                return current;
        }

        return NULL;
}
