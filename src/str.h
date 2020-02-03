#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct
{
	const char* ptr;
	size_t info;
} str;

#define str_lit(s) (str){ "" s, (sizeof(s) - 1) << 1 }
#define str_null (str){0}

str str_make(const char* const s, const size_t n);
str str_make_copy(const char* const s, const size_t n);
str str_lit_from_ptr(const char* s);

void str_assign(str* const dest, const str src);
str str_move(str* const src);
void str_free(const str s);

static inline
void str_clear(str* const s) { str_assign(s, str_null); }

static inline
size_t str_len(const str s) { return s.info >> 1; }

static inline
bool str_is_empty(const str s) { return str_len(s) == 0; }

static inline
bool str_is_lit(const str s) { return (s.info & 1) == 0; }

static inline
const char* str_ptr(const str s) { return s.ptr ? s.ptr : ""; }

int str_cmp(const str s1, const str s2);

static inline
bool str_eq(const str s1, const str s2) { return str_cmp(s1, s2) == 0; }

// string list
typedef struct
{
	size_t len, cap;
	str strings[];
} str_list;

str_list* str_list_append(str_list* list, const str s);
void str_list_free(str_list* const list);
void str_list_sort(str_list* const list);

static inline
bool str_list_is_empty(const str_list* const list) { return !list || (list->len == 0); }