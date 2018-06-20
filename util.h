/* xml2json
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */

#ifndef XML2JSON_UTIL_H
#define XML2JSON_UTIL_H

#include <arpa/inet.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void *xmalloc(size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern char *xstrdup(const char *s);

/*
 * Macros to guard against integer overflows.
 */
#define bitsizeof(x)  (CHAR_BIT * sizeof(x))

#define maximum_signed_value_of_type(a) \
    (INTMAX_MAX >> (bitsizeof(intmax_t) - bitsizeof(a)))

#define maximum_unsigned_value_of_type(a) \
    (UINTMAX_MAX >> (bitsizeof(uintmax_t) - bitsizeof(a)))

/*
 * Signed integer overflow is undefined in C, so here's a helper macro
 * to detect if the sum of two integers will overflow.
 *
 * Requires: a >= 0, typeof(a) equals typeof(b)
 */
#define signed_add_overflows(a, b) \
    ((b) > maximum_signed_value_of_type(a) - (a))

#define unsigned_add_overflows(a, b) \
    ((b) > maximum_unsigned_value_of_type(a) - (a))

/*
 * Returns true if the multiplication of "a" and "b" will
 * overflow. The types of "a" and "b" must match and must be unsigned.
 * Note that this macro evaluates "a" twice!
 */
#define unsigned_mult_overflows(a, b) \
    ((a) && (b) > maximum_unsigned_value_of_type(a) / (a))

#define xfree(ptr) do {                                 \
                if (ptr) { free(ptr); ptr = NULL; }     \
        } while (0)

#define ENSURE_NON_NULL(p) (p)?(p):""

#define ALLOC_ARRAY(x, alloc) (x) = xmalloc(st_mult(sizeof(*(x)), (alloc)))
#define REALLOC_ARRAY(x, alloc) (x) = xrealloc((x), st_mult(sizeof(*(x)), (alloc)))

#define alloc_nr(x) (((x)+16)*3/2)

/*
 * Realloc the buffer pointed at by variable 'x' so that it can hold
 * at least 'nr' entries; the number of entries currently allocated
 * is 'alloc', using the standard growing factor alloc_nr() macro.
 *
 * DO NOT USE any expression with side-effect for 'x', 'nr', or 'alloc'.
 */
#define ALLOC_GROW(x, nr, alloc) \
        do { \
                if ((nr) > alloc) { \
                        if (alloc_nr(alloc) < (nr)) \
                                alloc = (nr); \
                        else \
                                alloc = alloc_nr(alloc); \
                        REALLOC_ARRAY(x, alloc); \
                } \
        } while (0)


static inline size_t st_mult(size_t a, size_t b)
{
        if (unsigned_mult_overflows(a, b)) {
                fprintf(stderr, "size_t overflow: %zx * %zx",
                        (uintmax_t)a, (uintmax_t)b);
                exit(EXIT_FAILURE);
        }

        return a * b;
}

#ifdef __cplusplus
}
#endif

#endif  /* XML2JSON_UTIL_H */
