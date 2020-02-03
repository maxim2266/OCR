#define _POSIX_C_SOURCE 200809L

#include "page_spec.h"
#include "str.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <unistd.h>

static const char usage_string[] =
"Usage:\t" PROG_NAME " [OPTION]... [DIR]\n"
"List image or text files (aka pages) produced by ocr-* tools in directory DIR,\n"
"ordered by page number.\n\n"
"Options:\n"
"  -0,--null\n"
"         Output items are terminated by a null character instead of by newline.\n\n"
"  -t,--text\n"
"         List text files instead of images.\n\n"
"  -p,--pages=SPEC\n"
"         Pages to list. A page specification contains one or more comma-separated page\n"
"         ranges. A page range is either a page number, or two page numbers separated by\n"
"         a dash. In the last range, the second page number may be omitted, meaning all\n"
"         the remaining pages of the document. For instance, specification \"1-10\" outputs\n"
"         pages 1 to 10, and specification \"1,3,5-\" outputs pages 1 and 3, followed by\n"
"         all the pages starting from page 5 to the end of the document.\n"
"         (optional, default: all pages)\n\n"
"  -f,--fail-on-empty\n"
"         Fail if no files found.\n\n"
"  -h,--help\n"
"         Show help and exit.\n\n"
"  -v,--version\n"
"         Show version and exit.\n";

// command line parameters
typedef struct
{
	const char *dir, *ext;
	const page_spec* spec;
	char delim;
} command;

// option parser
static bool fail_on_empty = false;

static
void parse_options(command* const cmd, int argc, char** argv)
{
	// options specification
	static
	const struct option long_options[] =
	{
		{"null",  no_argument, NULL, '0'},
		{"text",  no_argument, NULL, 't'},
		{"pages",  required_argument, NULL, 'p'},
		{"fail-on-empty",  no_argument, NULL, 'f'},
		{"help",  no_argument, NULL, 'h'},
		{"version",  no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	// prepare target
	*cmd = (command){ .dir = ".", .ext = "pgm", .delim = '\n' };

	// parser loop
	int opt, option_index = 0;

	while((opt = getopt_long(argc, argv, "+0tp:fhv", long_options, &option_index)) >= 0)
	{
		switch(opt)
		{
			case '0':
				cmd->delim = 0;
				break;
			case 't':
				cmd->ext = "txt";
				break;
			case 'p':
				if(cmd->spec)
					free((void*)cmd->spec);

				if(!(cmd->spec = parse_page_spec(optarg)))
					die(0, "empty parameter specified for -p,--pages option");

				break;
			case 'f':
				fail_on_empty = true;
				break;
			case 'h':
				show_usage_and_exit(usage_string);
				break;
			case 'v':
				show_version_and_exit();
				break;
			case '?':
				exit(1);
			default:
				die(0, "internal error (getopt_long(3) returned %d)", opt);
		}
	}

	// directory
	switch(argc - optind)
	{
		case 0:
			break;
		case 1:
			cmd->dir = argv[optind];
			break;
		default:
			die(0, "cannot process more than one directory");
	}
}

// file list
static __attribute__((noreturn))
void child_proc(const command* const cmd, int pfd[2])
{
	_libc_int(close(pfd[0]), "close");				// unused read end
	_libc_int(dup2(pfd[1], STDOUT_FILENO), "dup2");	// dup write end to stdout
	_libc_int(close(pfd[1]), "close");				// close write end

	// format regex
	static
	const char regex_fmt[] = ".*/page-[0-9]{1,4}\\.%s$";

	char regex[64];

	sprintf(regex, regex_fmt, cmd->ext);

	// exec command
	execlp("find", "find", cmd->dir, "-maxdepth", "1", "-regextype", "posix-egrep",
		   "-type", "f", "-regex", regex, "-print0", NULL);

	// can only get here if the above exec() call has failed
	_die(errno, "internal error (execlp)");
}

static
str_list* read_file_list(const int fd)
{
	str_list* list = NULL;

	// open input stream
	FILE* const stream = libc_ptr(fdopen(fd, "r"), "fdopen");

	// read command output
	char* line = NULL;
	size_t cap = 0;
	ssize_t len;

	while((len = getdelim(&line, &cap, 0, stream)) >= 0)
		if(len > 0)
			list = str_list_append(list, str_make_copy(line, len));

	if(line)
		free(line);

	libc_int(fclose(stream), "fclose");

	return list;
}

static
unsigned page_no(const str name, const str ext)
{
	const char* const base = str_ptr(name);
	const char* s = base + str_len(name) - str_len(ext) - 1;

	if(s <= base || *s != '.')
		die(0, "internal error (page number from: \"%s\")", s);

	unsigned page_no = 0, k = 1;

	for(--s; s > base && isdigit(*s); --s, k *= 10)
		page_no += k * (*s - '0');

	if(*s != '-' || s == base)
		die(0, "internal error (page number from: \"%s\")", base);

	return page_no;
}

static
str_list* apply_spec(str_list* const src, const command* const cmd)
{
	if(!cmd->spec || str_list_is_empty(src))
		return src;

	const str ext = str_lit_from_ptr(cmd->ext);

	str_list* res = NULL;

	const str* const end = src->strings + src->len;

	for(str* s = src->strings; s < end; ++s)
		if(find_page_range(cmd->spec, page_no(*s, ext)))
			res = str_list_append(res, str_move(s));

	str_list_free(src);

	return res;
}

static
str_list* list_files(const command* const cmd)
{
	// pipe for stdout redirect
	int pfd[2];

	libc_int(pipe(pfd), "pipe");

	// fork
	fflush(NULL);	// flush all output streams

	const int pid = libc_int(fork(), "fork");

	if(pid == 0)	// child process
		child_proc(cmd, pfd);

	libc_int(close(pfd[1]), "close");	// write end

	// read file list
	str_list* list = read_file_list(pfd[0]);

	// wait for the child to terminate
	int status;

	libc_int(waitpid(pid, &status, 0), "waitpid");

	if(WIFEXITED(status))
	{
		if((status = WEXITSTATUS(status)) != 0)
			exit(status);
	}
	else if(WIFSIGNALED(status))
		die(0, "internal error (child terminated by signal %s)", strsignal(WTERMSIG(status)));

	// apply spec
	list = apply_spec(list, cmd);

	// sort
	str_list_sort(list);

	return list;
}

static
void print_list(const str_list* const list, char delim)
{
	struct iovec* const vec = mem_alloc(2 * list->len * sizeof(struct iovec));
	int len = 0;

	for(size_t i = 0; i < list->len; ++i)
	{
		const str s = list->strings[i];

		if(!str_is_empty(s))
		{
			vec[len++] = (struct iovec){ (void*)str_ptr(s), str_len(s) };
			vec[len++] = (struct iovec){ &delim, 1 };
		}
	}

	if(fflush(NULL) != 0)	// just in case
		die(errno, "internal error (fflush)");

	libc_int(writev(STDOUT_FILENO, vec, len), "writev");
	mem_free(vec);
}

int main(int argc, char** argv)
{
	command cmd;

	parse_options(&cmd, argc, argv);

	str_list* const list = list_files(&cmd);

	if(!str_list_is_empty(list))
		print_list(list, cmd.delim);
	else if(fail_on_empty)
		error(2, 0, "no files found");

	str_list_free(list);	// useless...

	return 0;
}