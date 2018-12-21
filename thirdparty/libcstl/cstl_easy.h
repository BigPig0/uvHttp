/*!
 * \file cstl_easy.c
 * \date 2018/12/13 14:02
 *
 * \author wlla
 * Contact: user@company.com
 *
 * \brief 
 *
 * TODO: libcstl�е����������ķ�װ��ʹ���롢���Ҳ������Ӽ�
 *
 * \note
*/

#ifndef CSTL_EASY_H
#define CSTL_EASY_H

#include "cstl.h"

#if ( defined _WIN32 )
#ifndef _THIRD_UTIL_API
#ifdef THIRD_UTIL_EXPORT
#define _THIRD_UTIL_API		_declspec(dllexport)
#else
#define _THIRD_UTIL_API		extern
#endif
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _THIRD_UTIL_API
#define _THIRD_UTIL_API
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

_THIRD_UTIL_API void hash_map_insert_easy(hash_map_t* phmap_map, const void* key, const void* value);

_THIRD_UTIL_API void map_insert_easy(map_t* pmap_map, const void* key, const void* value);

_THIRD_UTIL_API void string_map_compare(const void* cpv_first, const void* cpv_second, void* pv_output);

_THIRD_UTIL_API void string_map_hash(const void* cpv_input, void* pv_output);

_THIRD_UTIL_API void not_free_int(int n);

#define MAP_INSERT(_mapptr, _keytype, _key, _valuetype, _value) { \
    pair_t* pt_pair = create_pair(_keytype, _valuetype); \
    pair_init_elem(pt_pair, _key, _value); \
    map_insert(_mapptr, pt_pair); \
    pair_destroy(pt_pair); \
}

#define MAP_FOR_BEGIN(_mapptr, _keytype, _key, _valuetype, _value) \
    if (_mapptr) {\
        map_iterator_t it = map_begin(_mapptr);\
        map_iterator_t end = map_end(_mapptr);\
        pair_t* pt_pair;\
        _keytype _key;\
        _valuetype _value;\
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {\
            pt_pair = (pair_t*)iterator_get_pointer(it);\
            _key = *(_keytype*)pair_first(pt_pair);\
            _value = *(_valuetype*)pair_second(pt_pair);\

#define MAP_FOR_END }}

#define HASH_SET_FOR_BEGIN(_setptr, _type, _value) \
    if (_setptr) {\
        hash_set_iterator_t it = hash_set_begin(_setptr); \
        hash_set_iterator_t end = hash_set_end(_setptr); \
        _type _value;\
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {\
            _value = *(_type*)iterator_get_pointer(it);

#define HASH_SET_FOR_END }}
            

#define MAP_DESTORY(_mapptr, _keytype, _valuetype, _keydesfunc, _valuedesfunc) \
    if (_mapptr) {\
        map_iterator_t it = map_begin(_mapptr);\
        map_iterator_t end = map_end(_mapptr);\
        pair_t* pt_pair;\
        _keytype key;\
        _valuetype value;\
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {\
            pt_pair = (pair_t*)iterator_get_pointer(it);\
            key = *(_keytype*)pair_first(pt_pair);\
            value = *(_valuetype*)pair_second(pt_pair);\
            _keydesfunc(key);\
            _valuedesfunc(value);\
        }\
        map_destroy(_mapptr);\
    }

#define HASH_MAP_DESTORY(_mapptr, _keytype, _valuetype, _keydesfunc, _valuedesfunc) \
    if (_mapptr) {\
        hash_map_iterator_t it = hash_map_begin(_mapptr);\
        hash_map_iterator_t end = hash_map_end(_mapptr);\
        pair_t* pt_pair;\
        _keytype key;\
        _valuetype value;\
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {\
            pt_pair = (pair_t*)iterator_get_pointer(it);\
            key = *(_keytype*)pair_first(pt_pair);\
            value = *(_valuetype*)pair_second(pt_pair);\
            _keydesfunc(key);\
            _valuedesfunc(value);\
        }\
        hash_map_destroy(_mapptr);\
    }

#define VECTOR_DESTORY(_vecptr, _type, _desfunc) \
    if(_vecptr) {\
        vector_iterator_t it = vector_begin(_vecptr);\
        vector_iterator_t end = vector_end(_vecptr);\
        _type value;\
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {\
            value = *(_type*)iterator_get_pointer(it);\
            _desfunc(value);\
        }\
        vector_destroy(_vecptr);\
    }

#define SET_DESTORY(_setptr, _type, _desfunc) \
    if(_setptr) {\
        set_iterator_t it = set_begin(_setptr);\
        set_iterator_t end = set_end(_setptr);\
        _type value;\
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {\
            value = *(_type*)iterator_get_pointer(it);\
            _desfunc(value);\
        }\
        set_destroy(_setptr);\
    }

#define HASH_SET_DESTORY(_setptr, _type, _desfunc) \
    if(_setptr) {\
        hash_set_iterator_t it = hash_set_begin(_setptr);\
        hash_set_iterator_t end = hash_set_end(_setptr);\
        _type value;\
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {\
            value = *(_type*)iterator_get_pointer(it);\
            _desfunc(value);\
        }\
        hash_set_destroy(_setptr);\
    }

#define LIST_DESTORY(_listptr, _type, _desfunc) \
    if(_listptr) {\
        list_iterator_t it = list_begin(_listptr);\
        list_iterator_t end = list_end(_listptr);\
        _type value;\
        for (; iterator_not_equal(it, end); it = iterator_next(it)) {\
            value = *(_type*)iterator_get_pointer(it);\
            _desfunc(value);\
        }\
        list_destroy(_listptr);\
    }

#ifdef __cplusplus
}
#endif

#endif
