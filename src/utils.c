#include "utils.h"

#include <stdlib.h>
#include <stdio.h>

// memory allocation fuctions
void* mem_alloc(const size_t n)
{
	void* const p = malloc(n);

	if(!p)
		die_code(ENOMEM, "internal error");

	return p;
}

void* mem_realloc(void* p, const size_t n)
{
	if(n == 0)
		die("internal error: zero size in realloc(3)");

	if(!(p = realloc(p, n)))
		die_code(ENOMEM, "internal error");

	return p;
}

// program version display
#ifndef PROG_NAME
#error PROG_NAME is undefined
#endif

#ifndef PROG_VER
#error PROG_VER is undefined
#endif

void show_version_and_exit(void)
{
	fputs(PROG_NAME " " PROG_VER "\n", stderr);
	exit(1);
}
