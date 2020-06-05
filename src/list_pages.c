#include "list_pages.h"
#include "str.h"

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

// error check for child process
static
int _just_int_impl(const int ret, const char* const file, const int line)
{
	if(ret < 0)
	{
		error(0, errno, "internal error: file %s, line %d", file, line);
		_exit(1);
	}

	return ret;
}

#define _just(expr)	\
	_Generic((expr), 	\
		int:	_just_int_impl	\
	)((expr), __FILE__, __LINE__)

// file list
static
void child_proc(const char* const dir, const char* const ext, int pfd[2])
{
	_just(close(pfd[0]));				// unused read end
	_just(dup2(pfd[1], STDOUT_FILENO));	// dup write end to stdout
	_just(close(pfd[1]));				// close write end

	// format regex
	char regex[100];

	sprintf(regex, ".*/page-[0-9]{1,4}\\.%s$", ext);

	// exec command
	_just(execlp("find", program_invocation_name, dir, "-maxdepth", "1",
				 "-regextype", "posix-egrep", "-type", "f", "-regex", regex,
				 "-print0", NULL));
}

static
str_list* read_file_list(const int fd)
{
	str_list* list = NULL;

	// open input stream
	FILE* const stream = just(fdopen(fd, "r"));

	// read command output
	char* line = NULL;
	size_t cap = 0;
	ssize_t len;

	while((len = getdelim(&line, &cap, 0, stream)) >= 0)
		if(len > 0)
			list = str_list_append_copy(
					list,
					str_ref_chars(line, line[len - 1] == 0 ? (len - 1) : len));

	mem_free(line);
	just(fclose(stream));

	return list;
}

unsigned page_no(const str name, const str ext)
{
	if(!str_has_suffix(name, ext))
		die(0, "internal error (no extension \"%.*s\" in file name \"%.*s\")",
			(int)str_len(ext), str_ptr(ext),
			(int)str_len(name), str_ptr(name));

	const char* const base = str_ptr(name);
	const char* s = base + str_len(name) - str_len(ext) - 1;

	if(*s != '.')
		die(0, "internal error (unexpected extension in file name \"%.*s\")",
			(int)str_len(name), str_ptr(name));

	unsigned page_no = 0, k = 1;

	for(--s; s > base && *s >= '0' && *s <= '9'; --s, k *= 10)
		page_no += k * (*s - '0');

	if(*s != '-' || s == base || page_no == 0)
		die(0, "internal error (cannot get page number from file name \"%.*s\")",
			(int)str_len(name), str_ptr(name));

	return page_no;
}

static
str_list* apply_spec(str_list* const src,
					 const page_spec* const spec,
					 const char* const ext)
{
	if(!spec || str_list_is_empty(src))
		return src;

	const str e = str_ref(ext);
	str_list* res = NULL;
	const str* const end = src->strings + src->len;

	for(str* s = src->strings; s < end; ++s)
		if(find_page_range(spec, page_no(*s, e)))
			res = str_list_append(res, str_move(s));

	str_list_free(src);
	return res;
}

str_list* list_files(const char* const dir,
					 const page_spec* const spec,
					 const char* const ext)
{
	// flush all output streams
	just(fflush(NULL));

	// pipe for stdout redirect
	int pfd[2];

	just(pipe(pfd));

	// fork
	const int pid = just(fork());

	if(pid == 0)	// child process
		child_proc(dir, ext, pfd);

	just(close(pfd[1]));	// write end

	// read file list
	str_list* list = read_file_list(pfd[0]);

	// wait for the child to terminate
	int status;

	just(waitpid(pid, &status, 0));
	check_exit_status(status);

	// apply spec
	list = apply_spec(list, spec, ext);

	// sort
	if(list)
		str_sort_range(str_order_asc, list->strings, list->len);

	return list;
}
