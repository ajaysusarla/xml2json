/* xml2json
 *
 * Copyright (c) 2018 Partha Susarla <mail@spartha.org>
 */

#include "json.h"

#include "cstring.h"
#include "util.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


/*
 * Private Functions
 */
static void parse_json_object(JsonObject *object, cstring *str);

static bool is_json_type_valid(unsigned int type)
{
        return (type <= JSON_OBJECT);
}

static void parse_string_object(const char *s, cstring *str)
{
        /* TODO: Make this function utf-8 aware */
        cstring_addch(str, '"');
        cstring_addstr(str, s);
        cstring_addch(str, '"');
}

static void parse_num_object(double num, cstring *str)
{
        char buf[64];

        sprintf(buf, "%.16g", num);
        /* TODO: Check if `buf` has a valid number */
        cstring_addstr(str, buf);
}

static void parse_array_object(JsonObject *object, cstring *str)
{
        JsonObject *element;

        cstring_addch(str, '[');
        json_foreach(element, object) {
                parse_json_object(element, str);
                if (element->next != NULL)
                        cstring_addch(str, ',');
        }
        cstring_addch(str, ']');
}

static void parse_object(JsonObject *object, cstring *str)
{
        JsonObject *member;

        cstring_addch(str, '{');

        json_foreach(member, object) {
                parse_string_object(member->key, str);
                cstring_addch(str, ':');
                parse_json_object(member, str);
                if (member->next != NULL)
                        cstring_addch(str, ',');
        }

        cstring_addch(str, '}');
}

static void parse_json_object(JsonObject *object, cstring *str)
{
        assert(is_json_type_valid(object->type));

        switch (object->type) {
        case JSON_NULL:
                cstring_addstr(str, "null");
                break;
        case JSON_BOOL:
                cstring_addstr(str, object->bool_ ? "true" : "false");
                break;
        case JSON_STRING:
                parse_string_object(object->str_, str);
                break;
        case JSON_NUMBER:
                parse_num_object(object->num_, str);
                break;
        case JSON_ARRAY:
                parse_array_object(object, str);
                break;
        case JSON_OBJECT:
                parse_object(object, str);
                break;
        default:
                assert(false);
        }
}

static const char *json_object_to_string(JsonObject *object)
{
        cstring jsonstr;
        size_t len = 0;

        cstring_init(&jsonstr, 0);

        parse_json_object(object, &jsonstr);

        return cstring_detach(&jsonstr, &len);
}

static JsonObject *json_obj_new(JsonType type)
{
        JsonObject *obj = (JsonObject *) xcalloc(1, sizeof(JsonObject));

        obj->type = type;

        return obj;
}

static void json_remove_from_parent(JsonObject *obj)
{
        JsonObject *parent = obj->parent;

        if (parent) {
                if (obj->prev != NULL)
                        obj->prev->next = obj->next;
                else
                        parent->children.head = obj->next;

                if (obj->next != NULL)
                        obj->next->prev = obj->prev;
                else
                        obj->children.tail = obj->prev;

                free(obj->key);

                obj->parent = NULL;
                obj->prev = obj->next = NULL;
                obj->key = NULL;
        }
}

static void json_obj_free(JsonObject *obj)
{
        if (obj) {
                json_remove_from_parent(obj);

                switch(obj->type) {
                case JSON_STRING:
                        free(obj->str_);
                        break;
                case JSON_ARRAY:
                case JSON_OBJECT:
                {
                        JsonObject *child, *next;
                        for (child = obj->children.head; child != NULL; child = next) {
                                next = child->next;
                                json_obj_free(child);
                        }
                        break;
                }
                default:
                        break;
                }

                free(obj);
                obj = NULL;
        }
}

static void append_object(JsonObject *parent, JsonObject *child)
{
        child->parent = parent;
        child->prev = parent->children.tail;
        child->next = NULL;

        if (parent->children.tail != NULL)
                parent->children.tail->next = child;
        else
                parent->children.head = child;

        parent->children.tail = child;
}

static void prepend_object(JsonObject *parent, JsonObject *child)
{
        child->parent = parent;
        child->prev = NULL;
        child->next = parent->children.head;

        if (parent->children.head != NULL)
                parent->children.head->prev = child;
        else
                parent->children.tail = child;

        parent->children.head = child;
}

/*
 * Public Functions
 */

const char *json_encode(JsonObject *obj)
{
        return json_object_to_string(obj);
}

JsonObject *json_null_obj(void)
{
        return json_obj_new(JSON_NULL);
}

JsonObject *json_bool_obj(bool b)
{
        JsonObject *obj = json_obj_new(JSON_BOOL);
        obj->bool_ = b;
        return obj;
}

JsonObject *json_string_obj(const char *str)
{
        JsonObject *obj = json_obj_new(JSON_STRING);
        obj->str_ = xstrdup(str);
        return obj;
}

JsonObject *json_num_obj(double num)
{
        JsonObject *obj = json_obj_new(JSON_NUMBER);
        obj->num_ = num;
        return obj;
}

JsonObject *json_array_obj(void)
{
        return json_obj_new(JSON_ARRAY);
}

JsonObject *json_new(void)
{
        return json_obj_new(JSON_OBJECT);
}

void json_append_to_array(JsonObject *array, JsonObject *element)
{
        assert(array->type == JSON_ARRAY);
        assert(element->parent == NULL);

        append_object(array, element);
}

void json_prepend_to_array(JsonObject *array, JsonObject *element)
{
        assert(array->type == JSON_ARRAY);
        assert(element->parent == NULL);

        prepend_object(array, element);
}

void json_append_member(JsonObject *object, const char *key, JsonObject *value)
{
        assert(object->type == JSON_OBJECT);
        assert(value->parent == NULL);

        value->key = xstrdup(key);
        append_object(object, value);
}

void json_prepend_member(JsonObject *object, const char *key, JsonObject *value)
{
        assert(object->type == JSON_OBJECT);
        assert(value->parent == NULL);

        value->key = xstrdup(key);
        prepend_object(object, value);
}

bool json_validate(JsonObject *object)
{
        return false;
}

void json_free(JsonObject *obj)
{
        json_obj_free(obj);
}

JsonObject *json_first_child(JsonObject *object)
{
        if (object != NULL &&
            (object->type == JSON_ARRAY || object->type == JSON_OBJECT))
                return object->children.head;

        return NULL;
}
