#include "utils.h"

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

str_list* str_list_append_copy(str_list* list, const str s)
{
	str t = str_null;

	str_cpy(&t, s);

	return str_list_append(list, t);
}

void str_list_free(str_list* const list)
{
	if(list)
	{
		for(size_t i = 0; i < list->len; ++i)
			str_free(list->strings[i]);

		free(list);
	}
}
