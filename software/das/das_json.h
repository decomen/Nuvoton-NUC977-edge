#ifndef __DAS_JSON_H__
#define __DAS_JSON_H__

#include <stdint.h>

// use json-c lib
#include "json_object.h"
#include "json_tokener.h"
#include "json_util.h"
typedef struct json_object das_json_obj;

das_json_obj *das_json_from_file(const char * filename);

das_json_obj *das_json_new_obj(void);
void das_json_del_obj(das_json_obj *obj);
das_json_obj *das_json_parse(const char *str, int len);
const char *das_json_to_string(das_json_obj *obj);
das_json_obj *das_json_new_array(void);
void das_json_array_add(das_json_obj *array, das_json_obj *item);
int das_json_array_length(das_json_obj *obj);
das_json_obj *das_json_array_get_idx(das_json_obj *obj, int idx);

das_json_obj *das_json_new_array_ex(das_json_obj **obj_list, int count);
das_json_obj *das_json_new_array_single(das_json_obj *obj);

int das_json_obj_is_array(das_json_obj *obj);

das_json_obj *das_json_get_obj(das_json_obj *obj, const char *key);
int32_t das_json_get_int32(das_json_obj *obj, const char *key, int32_t def);
uint32_t das_json_get_uint32(das_json_obj *obj, const char *key, uint32_t def);
int64_t das_json_get_int64(das_json_obj *obj, const char *key, int64_t def);
uint64_t das_json_get_uint64(das_json_obj *obj, const char *key, uint64_t def);
double das_json_get_double(das_json_obj *obj, const char *key, double def);
const char *das_json_get_string(das_json_obj *obj, const char *key, const char *def);
const char *das_json_obj_string(das_json_obj *obj);

das_json_obj *das_json_new_int32(int32_t value);
das_json_obj *das_json_new_uint32(uint32_t value);
das_json_obj *das_json_new_int64(int64_t value);
das_json_obj *das_json_new_uint64(uint64_t value);
das_json_obj *das_json_new_double(double value);
das_json_obj *das_json_new_string(const char *value);

void das_json_add_obj(das_json_obj *obj, const char *key, das_json_obj *value);
void das_json_add_int32(das_json_obj *obj, const char *key, int32_t value);
void das_json_add_uint32(das_json_obj *obj, const char *key, uint32_t value);
void das_json_add_int64(das_json_obj *obj, const char *key, int64_t value);
void das_json_add_uint64(das_json_obj *obj, const char *key, uint64_t value);
void das_json_add_double(das_json_obj *obj, const char *key, double value);
void das_json_add_string(das_json_obj *obj, const char *key, const char *value);
void das_json_printf(das_json_obj *obj, const char *key, const char *fmt, ...);

#endif

