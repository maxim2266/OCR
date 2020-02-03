#include "utils.h"

#include <stdio.h>
#include <unistd.h>

// memory allocation fuctions
void* mem_alloc(const size_t n)
{
	return just(malloc(n));
}

void* mem_realloc(void* p, const size_t n)
{
	if(n == 0)
		die(0, "internal error: zero size in realloc(3)");

	return just(realloc(p, n));
}

// program version display
#ifndef PROG_NAME
#error constant PROG_NAME is undefined
#endif

#ifndef PROG_VER
#error constant PROG_VER is undefined
#endif

void show_version_and_exit(void)
{
	fputs(PROG_NAME " " PROG_VER "\n", stderr);
	exit(1);
}

void show_usage_and_exit(const char* const usage_string)
{
	fputs(usage_string, stderr);
	exit(1);
}
