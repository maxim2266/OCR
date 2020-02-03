#pragma once

#include <stdlib.h>
#include <error.h>
#include <errno.h>

// error exit
#define die(code, msg, ...) 	error(1, (code), "" msg, ##__VA_ARGS__)
#define _die(code, msg, ...)	do { error(0, (code), "" msg, ##__VA_ARGS__); _exit(1); } while(0)

// error checking
#define just(expr)	\
	_Generic((expr), 	\
			long:		_check_int_ret,	\
			int:		_check_int_ret,	\
			FILE*:		_check_ptr_ret,	\
			void*:		_check_ptr_ret,	\
			default: 	NULL	\
	)((expr), __FILE__, __LINE__)

static inline
int _check_int_ret(const int ret, const char* const file, const int line)
{
	if(ret < 0)
		error(1, errno, "internal error: file %s, line %d", file, line);

	return ret;
}

static inline
long _check_long_ret(const long ret, const char* const file, const int line)
{
	if(ret < 0)
		error(1, errno, "internal error: file %s, line %d", file, line);

	return ret;
}

static inline
void* _check_ptr_ret(void* const ret, const char* const file, const int line)
{
	if(!ret)
		error(1, errno, "internal error: file %s, line %d", file, line);

	return ret;
}

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

// error checks
int libc_int(const int ret, const char* const func_name);
void* libc_ptr(void* const ptr, const char* const func_name);
int _libc_int(const int ret, const char* const func_name);

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