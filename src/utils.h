#pragma once

#include <stdlib.h>
#include <error.h>
#include <errno.h>

// error termination
#define die_code(code, msg, ...) 	error(1, (code), "" msg, ##__VA_ARGS__)
#define die_errno(msg, ...) 		die_code(errno, msg, ##__VA_ARGS__)
#define die(msg, ...) 				die_code(0, msg, ##__VA_ARGS__)

// unused parameter
#define UNUSED(x) UNUSED_ ## x __attribute__((unused))

// memory allocation functions
void* mem_alloc(const size_t n) __attribute__((malloc));
void* mem_realloc(void* const p, const size_t n);

static inline
void mem_free(const void* const p)
{
	if(p)
		free((void*)p);
}

// min/max functions
// see https://gcc.gnu.org/onlinedocs/gcc/Typeof.html
#define max(a,b) 			\
({							\
	__auto_type _a = (a);	\
	__auto_type _b = (b);	\
	_a > _b ? _a : _b;		\
})

#define min(a,b)			\
({							\
	__auto_type _a = (a);	\
	__auto_type _b = (b);	\
	_a < _b ? _a : _b;		\
})

// program version display
void show_version_and_exit(void) __attribute__((noreturn));