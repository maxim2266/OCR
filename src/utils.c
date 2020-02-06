#define _GNU_SOURCE
#define _POSIX_C_SOURCE	200809L

#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// exit status check
void check_exit_status(int status)
{
	if(WIFEXITED(status))
	{
		if((status = WEXITSTATUS(status)) != 0)
			exit(status);
	}
	else if(WIFSIGNALED(status))
	{
		const int sig = WTERMSIG(status);

		die(0, "child process terminated by signal %d: %s", sig, strsignal(sig));
	}
}

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

// 'just' helpers
int _check_int_ret(const int ret, const char* const file, const int line)
{
	if(ret < 0)
		die(errno, "internal error: file %s, line %d", file, line);

	return ret;
}

long _check_long_ret(const ssize_t ret, const char* const file, const int line)
{
	if(ret < 0)
		die(errno, "internal error: file %s, line %d", file, line);

	return ret;
}

void* _check_ptr_ret(void* const ret, const char* const file, const int line)
{
	if(!ret)
		die(errno, "internal error: file %s, line %d", file, line);

	return ret;
}

