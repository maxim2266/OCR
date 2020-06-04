#define _GNU_SOURCE

#include "list_pages.h"

#include <limits.h>
#include <stdio.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/uio.h>

static const char usage_string[] =
	"Usage:\t" PROG_NAME " [OPTION]... [DIR]\n"
	"List image or text files (aka pages) produced by ocr-* tools, from the directory DIR,\n"
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

static
void print_list(const str_list* const list, char delim)
{
	struct iovec* const vec = mem_alloc(2 * list->len * sizeof(struct iovec));
	size_t len = 0;

	for(size_t i = 0; i < list->len; ++i)
	{
		const str s = list->strings[i];

		if(!str_is_empty(s))
		{
			vec[len++] = (struct iovec){ (void*)str_ptr(s), str_len(s) };
			vec[len++] = (struct iovec){ &delim, 1 };
		}
	}

	just(fflush(NULL));

	// write chunks of no more than IOV_MAX strings each
	struct iovec* s = vec;
	struct iovec* const end = s + len;

	while(s < end)
	{
		const size_t n = min(end - s, IOV_MAX);

		just(writev(STDOUT_FILENO, s, n));
		s += n;
	}

	mem_free(vec);
}

int main(int argc, char** argv)
{
	command cmd;

	parse_options(&cmd, argc, argv);

	// make sure stdin is closed on exec
	just(fcntl(STDIN_FILENO, F_SETFD, fcntl(STDIN_FILENO, F_GETFD) | FD_CLOEXEC));

	// get file list
	str_list* const list = list_files(cmd.dir, cmd.spec, cmd.ext);

	if(!str_list_is_empty(list))
		print_list(list, cmd.delim);
	else if(fail_on_empty)
		error(2, 0, "no files found");

	str_list_free(list);	// useless...

	return 0;
}
