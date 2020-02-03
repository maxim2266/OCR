#define _POSIX_C_SOURCE 200809L

#include "page_spec.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static
page_spec* add_page_range(page_spec* spec, const page_range range)
{
	if(!spec)
	{
		const unsigned cap = 3;

		spec = mem_alloc(sizeof(page_spec) + (cap * sizeof(page_range)));

		spec->len = 0;
		spec->cap = cap;
	}
	else if(spec->len == spec->cap)
	{
		const unsigned cap = 2 * spec->cap;

		spec = mem_realloc(spec, sizeof(page_spec) + (cap * sizeof(page_range)));

		spec->cap = cap;
	}

	spec->ranges[spec->len++] = range;

	return spec;
}

static
void die_bad_spec(const char* const s)
{
	die(0, "invalid page spec, problematic part: \"%s\"", s);
}

static
const char* other_digits(const char* s, unsigned* const p)
{
	for(const char* const base = s; (s - base) < 3; ++s)
	{
		const char c = *s;

		switch(c)
		{
			case '0' ... '9':
				*p = *p * 10 + c - '0';
				continue;
			case 0:
			case '-':
			case ',':
				return s;
			default:
				die_bad_spec(s);
		}
	}

	return s;
}

static
const char* read_page_no(const char* s, unsigned* const p)
{
	const char c = *s;

	switch(c)
	{
		case '1' ... '9':
			*p = c - '0';
			return other_digits(s + 1, p);
		default:
			die_bad_spec(s);
	}

	abort(); // unreachable
}

static
page_spec* parse_spec(const char* s)
{
	if(!s || !*s)
		return NULL;

	page_spec* spec = NULL;
	page_range range;

	for(;;)
	{
		// first page number
		s = read_page_no(s, &range.first);

		// last page number, if any
		if(*s == '-')
		{
			if(*++s != 0)
				s = read_page_no(s, &range.last);
			else
				range.last = MAX_PAGE_NO;
		}
		else
			range.last = range.first;

		// add spec
		if(range.first > range.last)
			die(0, "invalid page range: %u-%u", range.first, range.last);

		spec = add_page_range(spec, range);

		// loop condition
		switch(*s)
		{
			case 0:
				return spec;
			case ',':
				++s;
				continue;
			default:
				die_bad_spec(s);
		}
	}
}

static
int by_first_page_no(const void* r1, const void* r2)
{
	return ((const page_range*)r1)->first - ((const page_range*)r2)->first;
}

page_spec* parse_page_spec(const char* s)
{
	page_spec* const spec = parse_spec(s);

	if(spec && spec->len > 1)
	{
		qsort(spec->ranges, spec->len, sizeof(spec->ranges[0]), by_first_page_no);

		// merge ranges
		page_range* curr = spec->ranges;
		const page_range* const end = spec->ranges + spec->len;

		for(const page_range* next = curr + 1; next < end; ++next)
		{
			if(next->first <= curr->last + 1)
				curr->last = max(next->last, curr->last);
			else if(++curr != next)
				*curr = *next;
		}

		// update length
		spec->len = curr - spec->ranges + 1;
	}

	return spec;
}

char* page_spec_to_string(const page_spec* const spec, size_t* const plen)
{
	char* str = NULL;
	size_t n = 0;

	// check the spec
	if(spec && spec->len > 0)
	{
		// memory stream
		FILE* const ms = just(open_memstream(&str, &n));

		// iterate the spec ranges
		const page_range* p = spec->ranges;
		const page_range* const end = p + spec->len;

		just(fprintf(ms, "%u-%u", p->first, p->last));

		for(++p; p < end; ++p)
			just(fprintf(ms, ",%u-%u", p->first, p->last));

		just(fclose(ms));
	}

	if(plen)
		*plen = n;

	return str;
}

static
int page_in_range(const void* key, const void* item)
{
	const unsigned page_no = (unsigned)(uintptr_t)key;
	const page_range* const range = (const page_range*)item;

	if(page_no < range->first)
		return -1;

	if(page_no > range->last)
		return 1;

	return 0;
}

const page_range* find_page_range(const page_spec* const spec, const unsigned page_no)
{
	return bsearch((const void*)(uintptr_t)page_no,
				   spec->ranges, spec->len, sizeof(page_range),
				   page_in_range);
}

#ifdef EXTEND_PRINTF
// extension for printf to print page_spec values
// see https://www.gnu.org/software/libc/manual/html_node/Printf-Extension-Example.html

#include <printf.h>

static
int print_page_spec(FILE* f, const struct printf_info *info, const void *const *args)
{
	const int pad = (info->left ? -info->width : info->width);
	const char* const str = page_spec_to_string(*((const page_spec**)args[0]), NULL);

	if(!str)
		return fprintf(f, "%*s", pad, "");

	const int ret = fprintf(f, "%*s", pad, str);

	free((void*)str);

	return ret;
}

static
int page_spec_arginfo(const struct printf_info* UNUSED(info), size_t n, int* argtypes, int* UNUSED(size))
{
	// exactly one argument of type const page_spec*
	if(n > 0)
		argtypes[0] = PA_POINTER;

	return 1;
}

static __attribute__((constructor(50000)))
void register_page_spec_format(void)
{
	register_printf_specifier('P', print_page_spec, page_spec_arginfo);
}

#endif	// EXTEND_PRINTF
