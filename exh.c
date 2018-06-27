/* example htable usage
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */

#include "htable.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>

struct exh_htable {
        struct htable table;
};

struct exh_htable_entry {
        struct htable_entry entry;
        unsigned char *key;
        size_t keylen;
        void *value;
};

static struct exh_htable_entry *alloc_exh_htable_entry(unsigned char *key,
                                                       size_t keylen,
                                                       void *value)
{
        struct exh_htable_entry *e;

        e = xmalloc(sizeof(struct exh_htable_entry));
        e->key = xmalloc(sizeof(unsigned char) * keylen);
        memcpy(e->key, key, keylen);
        e->keylen = keylen;
        e->value = value;

        return e;
}

static void free_exh_htable_entry(struct exh_htable_entry **e)
{
        if (e && *e) {
                free((*e)->key);
                (*e)->key = NULL;
                (*e)->keylen = 0;
                (*e)->value = NULL;
                free(*e);
                *e = NULL;
        }
}

static int exh_htable_entry_cmpfn(const void *unused1 __attribute__((unused)),
                                  const void *entry1,
                                  const void *entry2,
                                  const void *unused2 __attribute__((unused)))
{
        const struct exh_htable_entry *e1 = entry1;
        const struct exh_htable_entry *e2 = entry2;

        return memcmp_raw(e1->key, e1->keylen, e2->key, e2->keylen);
}

static void exh_htable_init(struct exh_htable *ht)
{
        htable_init(&ht->table, exh_htable_entry_cmpfn, NULL, 0);
}

static void *exh_htable_get(struct exh_htable *ht, unsigned char *key,
                            size_t keylen)
{
        struct exh_htable_entry k;
        struct exh_htable_entry *e;

        if (!ht->table.size)
                exh_htable_init(ht);

        htable_entry_init(&k, bufhash(key, keylen));
        k.key = (unsigned char *)key;
        k.keylen = keylen;
        e = htable_get(&ht->table, &k, NULL);

        return e ? e : NULL;
}

static void exh_htable_put(struct exh_htable *ht, unsigned char *key,
                           size_t keylen, void *value)
{
        struct exh_htable_entry *e;

        if (!ht->table.size)
                exh_htable_init(ht);

        e = alloc_exh_htable_entry(key, keylen, value);
        htable_entry_init(e, bufhash(key, keylen));

        htable_put(&ht->table, e);
}

static void *exh_htable_remove(struct exh_htable *ht, unsigned char *key,
                               size_t keylen)
{
        struct exh_htable_entry e;

        if (!ht->table.size)
                exh_htable_init(ht);

        htable_entry_init(&e, bufhash(key, keylen));

        return htable_remove(&ht->table, &e, key);
}

static void exh_htable_free(struct exh_htable *ht)
{
        struct htable_iter iter;
        struct exh_htable_entry *e;

        htable_iter_init(&ht->table, &iter);
        while ((e = htable_iter_next(&iter))) {
                free_exh_htable_entry(&e);
        }

        htable_free(&ht->table, 0);
}

int main(int argc, char **argv)
{
        struct exh_htable ht;
        struct htable_iter iter;
        struct exh_htable_entry *e;

        exh_htable_init(&ht);

        exh_htable_put(&ht, "foo", strlen("foo"), "value1");
        exh_htable_put(&ht, "bar", strlen("bar"), "value2");
        exh_htable_put(&ht, "partha", strlen("partha"), "value3");
        exh_htable_put(&ht, "norah", strlen("norah"), "value4");
        exh_htable_put(&ht, "foo", strlen("foo"), "value5");
        exh_htable_put(&ht, "fastmail", strlen("fastmail"), "value6");

        printf("First element at pos:%d\n", ht.table.iter_head);
        printf("Last element at pos:%d\n", ht.table.iter_tail);
        htable_iter_init(&ht.table, &iter);
        while ((e = htable_iter_next(&iter))) {
                printf("[pos:%d][p:%d][n:%d]%s = %s\n", iter.pos, e->entry.iter_prev,
                       e->entry.iter_next, e->key, (char *)e->value);
        }

        htable_iter_init_ordered(&ht.table, &iter);
        while ((e = htable_iter_next_ordered(&iter))) {
                printf("%s = %s\n", e->key, (char *)e->value);
                if (e->entry.count > 1) {
                        while ((e = htable_get_next(&ht.table, e))) {
                                printf("%s = %s\n", e->key, (char *)e->value);
                        }
                }
        }

        exh_htable_free(&ht);

        exit(EXIT_SUCCESS);
}
