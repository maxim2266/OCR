#pragma once

#include "utils.h"

// range of pages
typedef struct
{
	unsigned first, last;
} page_range;

// max. page number
#define MAX_PAGE_NO 9999

// page specification
typedef struct
{
	size_t len, cap;
	page_range ranges[];
} page_spec;

// page spec functions
page_spec* parse_page_spec(const char* s);

#define free_page_spec(spec)	mem_free((void*)(spec))

char* page_spec_to_string(const page_spec* const spec, size_t* const plen);
