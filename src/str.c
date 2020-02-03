#include "str.h"
#include "utils.h"

#include <string.h>

str str_make(const char* const s, const size_t n)
{
	return (!s || *s == 0 || n == 0) ? str_null : (str){ s, n << 1 };
}

str str_make_copy(const char* const s, const size_t n)
{
	if(!s || *s == 0 || n == 0)
		return str_null;

	char* const begin = mem_alloc(n + 1);
	char* const end = stpncpy(begin, s, n);

	*end = 0;

	return (str){ begin, ((end - begin) << 1) | 1 };
}

str str_lit_from_ptr(const char* s)
{
	const size_t n = s ? strlen(s) : 0;

	return (n > 0) ? (str){ s, n << 1 } : str_null;
}

void str_assign(str* const dest, const str src)
{
	if(dest)
	{
		str_free(*dest);
		*dest = src;
	}
}

str str_move(str* const src)
{
	if(!src)
		return str_null;

	str tmp = *src;

	*src = str_null;

	return tmp;
}

void str_free(const str s)
{
	if(!str_is_lit(s))
		mem_free(s.ptr);
}

int str_cmp(const str s1, const str s2)
{
	return strcmp(str_ptr(s1), str_ptr(s2));
}


// string list functions
#define STR_LIST_INITIAL_CAP 32

str_list* str_list_append(str_list* list, const str s)
{
	if(!list)
	{
		list = mem_alloc(sizeof(str_list) + STR_LIST_INITIAL_CAP * sizeof(str));

		list->len = 0;
		list->cap = STR_LIST_INITIAL_CAP;
	}
	else if(list->len == list->cap)
	{
		if(list->cap >= 1024 * 1024)
			list->cap += list->cap / 2;
		else
			list->cap *= 2;

		list = mem_realloc(list, sizeof(str_list) + list->cap * sizeof(str));
	}

	list->strings[list->len++] = s;

	return list;
}

void str_list_free(str_list* const list)
{
	if(list)
	{
		for(size_t i = 0; i < list->len; ++i)
			str_free(list->strings[i]);

		mem_free(list);
	}
}

static
int str_cmp_func(const void* s1, const void* s2)
{
	return str_cmp(*(const str*)s1, *(const str*)s2);
}

void str_list_sort(str_list* const list)
{
	if(list && list->len > 1)
		qsort(list->strings, list->len, sizeof(str), str_cmp_func);
}
