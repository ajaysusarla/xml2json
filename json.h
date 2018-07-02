/* xml2json
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */
#ifndef XML2JSON_JSON_H_
#define XML2JSON_JSON_H_

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        JSON_NULL,
        JSON_BOOL,
        JSON_STRING,
        JSON_NUMBER,
        JSON_ARRAY,
        JSON_OBJECT,
} JsonType;


typedef struct _JsonObject JsonObject;
struct _JsonObject {
        JsonObject *parent;
        JsonObject *prev;
        JsonObject *next;

        char *key;

        JsonType type;

        union {
                bool bool_;     /* JSON_BOOL */
                char *str_;     /* JSON_STRING */
                double num_;    /* JSON_NUMBER */
                struct {        /* JSON_ARRAY * JSON_OBJECT */
                        JsonObject *head;
                        JsonObject *tail;
                } children;
        };
};

extern char *json_encode(JsonObject *obj);

extern JsonObject *json_null_obj(void);
extern JsonObject *json_bool_obj(bool b);
extern JsonObject *json_string_obj(const char *str);
extern JsonObject *json_num_obj(double num);
extern JsonObject *json_array_obj(void);
extern JsonObject *json_new(void);
extern void json_free(JsonObject *obj);

/* array handler */
extern void json_append_to_array(JsonObject *array, JsonObject *element);
extern void json_prepend_to_array(JsonObject *array, JsonObject *element);

/* object handler */
extern void json_append_member(JsonObject *object, const char *key,
                               JsonObject *value);
extern void json_prepend_member(JsonObject *object, const char *key,
                                JsonObject *value);

extern bool json_validate(JsonObject *object);

/* Iterators */
extern JsonObject *json_first_child(JsonObject *object);

#define json_foreach(i, object) \
        for ((i) = json_first_child(object); \
             (i) != NULL; \
             (i) = (i)->next)

#ifdef __cplusplus
}
#endif

#endif  /* XML2JSON_JSON_H_ */
