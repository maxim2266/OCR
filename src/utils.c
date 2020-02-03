#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// memory allocation fuctions
void* mem_alloc(const size_t n)
{
	return libc_ptr(malloc(n), "malloc");
}

void* mem_realloc(void* p, const size_t n)
{
	if(n == 0)
		die(0, "internal error: zero size in realloc(3)");

	return libc_ptr(realloc(p, n), "realloc");
}

// error checks
int libc_int(const int ret, const char* const func_name)
{
	if(ret < 0)
		die(errno, "internal error (%s)", func_name);

	return ret;
}

void* libc_ptr(void* const ptr, const char* const func_name)
{
	if(!ptr)
		die(errno, "internal error (%s)", func_name);

	return ptr;
}


int _libc_int(const int ret, const char* const func_name)
{
	if(ret < 0)
		_die(errno, "internal error (%s)", func_name);

	return ret;
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
