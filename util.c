/* xml2json
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */

#include "util.h"

void *xmalloc(size_t size)
{
        void *ret;

        ret = malloc(size);
        if (!ret && !size)
                ret = malloc(1);

        if (!ret) {
                fprintf(stderr, "Out of memory. malloc failed.\n");
                exit(EXIT_FAILURE);
        }

        return ret;
}


void *xrealloc(void *ptr, size_t size)
{
        void *ret;

        ret = realloc(ptr, size);
        if (!ret && !size)
                ret = realloc(ptr, 1);

        if (!ret) {
                fprintf(stderr, "Out of memory. realloc failed.\n");
                exit(EXIT_FAILURE);
        }

        return ret;
}

void *xcalloc(size_t nmemb, size_t size)
{
        void *ret = NULL;

        if (!nmemb || !size)
                return ret;

        if (((size_t) - 1) / nmemb <= size) {
                fprintf(stderr, "Memory allocation error\n");
                exit(EXIT_FAILURE);
        }

        ret = (void *)calloc(nmemb, size);
        if (!ret) {
                fprintf(stderr, "Memory allocation error\n");
                exit(EXIT_FAILURE);
        }

        return ret;


}

char *xstrdup(const char *s)
{
        size_t len = strlen(s) + 1;
        char *ptr = xmalloc(len);

        memcpy(ptr, s, len);

        return ptr;
}
