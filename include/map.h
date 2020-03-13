/*
 * =======================================================================================
 *
 *      Filename:  map.h
 *
 *      Description:  Header File for C Hashmap
 *
 *      Version:   5.0
 *      Released:  10.11.2019
 *
 *      Author:   Thomas Gruber (tg), thomas.gruber@googlemail.com
 *      Project:  likwid
 *
 *      Copyright (C) 2019 RRZE, University Erlangen-Nuremberg
 *
 *      This program is free software: you can redistribute it and/or modify it under
 *      the terms of the GNU General Public License as published by the Free Software
 *      Foundation, either version 3 of the License, or (at your option) any later
 *      version.
 *
 *      This program is distributed in the hope that it will be useful, but WITHOUT ANY
 *      WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 *      PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License along with
 *      this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * =======================================================================================
 */

#ifndef MAP_H
#define MAP_H

#include <ghash.h>
#include <ghash_add.h>

typedef void* mpointer;
typedef void (*map_value_destroy_func)(mpointer data);
typedef void (*map_foreach_func)(mpointer key, mpointer value, mpointer user_data);

typedef enum {
    MAP_KEY_TYPE_STR = 0,
    MAP_KEY_TYPE_INT,
    MAP_KEY_TYPE_BSTR,
    MAX_MAP_KEY_TYPE
} MapKeyType;

typedef struct {
    mpointer key;
    mpointer value;
    mpointer iptr;
} MapValue;

typedef struct {
    int num_values;
    int size;
    int max_size;
    int id;
    GHashTable *ghash;
    MapKeyType key_type;
    MapValue *values;
    map_value_destroy_func value_func;
} Map;

typedef Map* Map_t;

int init_map(Map_t* map, MapKeyType type, int max_size, map_value_destroy_func value_func);
int get_map_size(Map_t map);

int init_smap(Map_t* map, map_value_destroy_func func);
int add_smap(Map_t map, char* key, void* val);
int update_smap(Map_t map, char* key, void* val, void** old);
int get_smap_by_key(Map_t map, char* key, void** val);
int get_smap_by_idx(Map_t map, int idx, void** val);
void foreach_in_smap(Map_t map, map_foreach_func func, mpointer user_data);
int del_smap(Map_t map, char* key);
int get_smap_size(Map_t map);
void destroy_smap(Map_t map);

int init_imap(Map_t* map, map_value_destroy_func value_func);
int add_imap(Map_t map, int64_t key, void* val);
int get_imap_by_key(Map_t map, int64_t key, void** val);
int get_imap_by_idx(Map_t map, int idx, void** val);
void foreach_in_imap(Map_t map, map_foreach_func func, mpointer user_data);
int del_imap(Map_t map, int64_t key);
int get_imap_size(Map_t map);
void destroy_imap(Map_t map);


#endif
