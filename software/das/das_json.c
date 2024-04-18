
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "das_json.h"

// use json-c lib
das_json_obj *das_json_new_obj(void)
{
    return json_object_new_object();
}

void das_json_del_obj(das_json_obj *obj)
{
    if (obj) json_object_put(obj);
}

das_json_obj *das_json_from_file(const char * filename)
{
    return  (das_json_obj *)json_object_from_file(filename);
}

das_json_obj *das_json_parse(const char *str, int len)
{
    if (str && len > 0) {
        struct json_tokener *tok = json_tokener_new();
        struct json_object *root_obj = json_tokener_parse_ex(tok, str, len);
        json_tokener_free(tok);
        return root_obj;
    }
    return NULL;
}

const char *das_json_to_string(das_json_obj *obj)
{
    return obj ? json_object_to_json_string(obj) : NULL;
}

das_json_obj *das_json_new_array(void)
{
    return json_object_new_array();
}

void das_json_array_add(das_json_obj *array, das_json_obj *item)
{
    if (array && item) {
        json_object_array_add(array, item);
    }
}

das_json_obj *das_json_new_array_ex(das_json_obj **obj_list, int count)
{
    das_json_obj *obj_arry = das_json_new_array();
    if (obj_arry) {
        int n;
        for (n = 0; n < count; n++) {
            das_json_array_add(obj_arry, obj_list[n]);
        }
    }
    return obj_arry;
}

das_json_obj *das_json_new_array_single(das_json_obj *obj)
{
    return das_json_new_array_ex(&obj, 1);
}

int das_json_obj_is_array(das_json_obj *obj)
{
    return obj ? json_type_array == json_object_get_type(obj) : 0;
}

int das_json_array_length(das_json_obj *obj)
{
    return obj ? json_object_array_length(obj) : -1;
}

das_json_obj *das_json_array_get_idx(das_json_obj *obj, int idx)
{
    return obj ? json_object_array_get_idx(obj, idx) : NULL;
}

das_json_obj *das_json_get_obj(das_json_obj *obj, const char *key)
{
    return obj && key ? json_object_object_get(obj, key) : NULL;
}

int32_t das_json_get_int32(das_json_obj *obj, const char *key, int32_t def)
{
    int32_t value = def;
    struct json_object *tmp = das_json_get_obj(obj, key);
    if (tmp) {
        enum json_type type = json_object_get_type(tmp);
        if (type == json_type_boolean || 
            type == json_type_double || 
            type == json_type_int || 
            type == json_type_string)
        value = json_object_get_int(tmp);
    }
    return value;
}

uint32_t das_json_get_uint32(das_json_obj *obj, const char *key, uint32_t def)
{
    return (uint32_t)das_json_get_int32(obj, key, (int32_t)def);
}

int64_t das_json_get_int64(das_json_obj *obj, const char *key, int64_t def)
{
    int64_t value = def;
    struct json_object *tmp = das_json_get_obj(obj, key);
    if (tmp) {
        enum json_type type = json_object_get_type(tmp);
        if (type == json_type_boolean || 
            type == json_type_double || 
            type == json_type_int || 
            type == json_type_string)
        value = json_object_get_int64(tmp);
    }
    return value;
}

uint64_t das_json_get_uint64(das_json_obj *obj, const char *key, uint64_t def)
{
    return (uint64_t)das_json_get_int64(obj, key, (int64_t)def);
}

double das_json_get_double(das_json_obj *obj, const char *key, double def)
{
    double value = def;
    struct json_object *tmp = das_json_get_obj(obj, key);
    if (tmp) {
        enum json_type type = json_object_get_type(tmp);
        if (type == json_type_boolean || 
            type == json_type_double || 
            type == json_type_int || 
            type == json_type_string)
        value = json_object_get_double(tmp);
    }
    return value;
}

const char *das_json_get_string(das_json_obj *obj, const char *key, const char *def)
{
    const char *value = def;
    struct json_object *tmp = das_json_get_obj(obj, key);
    if (tmp) {
        value = json_object_get_string(tmp);
    }
    return value;
}

const char *das_json_obj_string(das_json_obj *obj)
{
    return json_object_get_string(obj);
}

das_json_obj *das_json_new_int32(int32_t value)
{
    return (das_json_obj *)json_object_new_int(value);
}

das_json_obj *das_json_new_uint32(uint32_t value)
{
    return (das_json_obj *)json_object_new_int64((int64_t)value);
}

das_json_obj *das_json_new_int64(int64_t value)
{
    return (das_json_obj *)json_object_new_int64(value);
}

das_json_obj *das_json_new_uint64(uint64_t value)
{
    return (das_json_obj *)json_object_new_int64((int64_t)value);
}

das_json_obj *das_json_new_double(double value)
{
    return (das_json_obj *)json_object_new_double(value);
}

das_json_obj *das_json_new_string(const char *value)
{
    return (das_json_obj *)json_object_new_string((const char *)value);
}

void das_json_add_obj(das_json_obj *obj, const char *key, das_json_obj *value)
{
    if (obj && key)
        json_object_object_add(obj, key, value);
}

void das_json_add_int32(das_json_obj *obj, const char *key, int32_t value)
{
    if (obj && key)
        das_json_add_obj(obj, key, json_object_new_int(value));
}

void das_json_add_uint32(das_json_obj *obj, const char *key, uint32_t value)
{
    if (obj && key)
        das_json_add_obj(obj, key, json_object_new_int((int32_t)value));
}

void das_json_add_int64(das_json_obj *obj, const char *key, int64_t value)
{
    if (obj && key)
        das_json_add_obj(obj, key, json_object_new_int64(value));
}

void das_json_add_uint64(das_json_obj *obj, const char *key, uint64_t value)
{
    if (obj && key)
        das_json_add_obj(obj, key, json_object_new_int64((int64_t)value));
}

void das_json_add_double(das_json_obj *obj, const char *key, double value)
{
    if (obj && key)
        das_json_add_obj(obj, key, json_object_new_double(value));
}

void das_json_add_string(das_json_obj *obj, const char *key, const char *value)
{
    if (obj && key)
        das_json_add_obj(obj, key, value ? json_object_new_string((const char *)value) : NULL);
}

void das_json_printf(das_json_obj *obj, const char *key, const char *fmt, ...)
{
    char value[256] = "";
	va_list arp;
    
    va_start(arp, fmt);
    vsnprintf(value, sizeof(value), fmt, arp);
    va_end(arp);
    
    das_json_add_string(obj, key, value);
}

