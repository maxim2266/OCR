#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <unistd.h>

// error exit
#define die(code, msg, ...) 	error(1, (code), "" msg, ##__VA_ARGS__)

// error checking
#define just(expr)	\
	_Generic((expr), 	\
			ssize_t:	_check_long_ret,	\
			int:		_check_int_ret,	\
			FILE*:		_check_ptr_ret,	\
			void*:		_check_ptr_ret,	\
			default: 	NULL	\
	)((expr), __FILE__, __LINE__)

// 'just' helpers
int _check_int_ret(const int ret, const char* const file, const int line);
long _check_long_ret(const ssize_t ret, const char* const file, const int line);
void* _check_ptr_ret(void* const ret, const char* const file, const int line);

// exit status check
void check_exit_status(int status);

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

// program usage display
void show_usage_and_exit(const char* const usage_string) __attribute__((noreturn));