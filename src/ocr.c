#include "utils.h"
#include "tesseract.h"
#include "page_spec.h"
#include "list_pages.h"

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>

#define info(fmt, ...) just(printf("%s: " fmt "\n", program_invocation_name, ##__VA_ARGS__))

static const char usage_string[] =
	"Usage:\t" PROG_NAME " [OPTION]... [ -- TESSERACT-OPTION...]\n\n"
	"Run OCR on the specified range of pages.\n\n"
	"Options:\n"
	"  -p,--pages=SPEC\n"
	"         Pages to list. A page specification contains one or more comma-separated page\n"
	"         ranges. A page range is either a page number, or two page numbers separated by\n"
	"         a dash. In the last range, the second page number may be omitted, meaning all\n"
	"         the remaining pages of the document. For instance, specification \"1-10\" outputs\n"
	"         pages 1 to 10, and specification \"1,3,5-\" outputs pages 1 and 3, followed by\n"
	"         all the pages starting from page 5 to the end of the document.\n"
	"         (optional, default: all pages)\n\n"
	"  -d,--dir=DIR\n"
	"         Input directory (optional, default: .)\n\n"
	"  -f,--fail-on-empty\n"
	"         Fail if no files found.\n\n"
	"  -h,--help\n"
	"         Show help and exit.\n\n"
	"  -v,--version\n"
	"         Show version and exit.\n";

// option parser
typedef struct
{
	const char* dir;
	page_spec* spec;
	bool fail_on_empty;
	const char** tess_argv;
	unsigned tess_argc;
} command;

static
void parse_options(command* const cmd, int argc, char* argv[])
{
	// options specification
	static
	const struct option long_options[] =
	{
		{"pages",  required_argument, NULL, 'p'},
		{"dir",  required_argument, NULL, 'd'},
		{"fail-on-empty",  no_argument, NULL, 'f'},
		{"help",  no_argument, NULL, 'h'},
		{"version",  no_argument, NULL, 'v'},
		{NULL, 0, NULL, 0}
	};

	// prepare target
	*cmd = (command){ .dir = "." };

	// parser loop
	int opt, option_index = 0;

	while((opt = getopt_long(argc, argv, "+p:d:fhv", long_options, &option_index)) >= 0)
	{
		switch(opt)
		{
			case 'p':
				if(cmd->spec)
					free((void*)cmd->spec);

				if(!(cmd->spec = parse_page_spec(optarg)))
					die(0, "empty parameter specified for -p,--pages option");

				break;
			case 'd':
				if(*optarg == 0)
					die(0, "empty directory name");

				cmd->dir = optarg;
				break;
			case 'f':
				cmd->fail_on_empty = true;
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

	// tesseract options
	if(argc > optind)
	{
		cmd->tess_argv = (const char**)(argv + optind);
		cmd->tess_argc = argc - optind;
	}
}

static
void check_tess_lang_opt(const char** args, const unsigned num_args)
{
	// find "-l" option
	unsigned opt_ind = 0;

	while(opt_ind < num_args && strcmp(args[opt_ind], "-l") != 0)
		++opt_ind;

	if(opt_ind == num_args)
		return;

	// get option argument
	if(++opt_ind == num_args)
		die(0, "missing argument for tesseract option -l");

	const char* const spec = args[opt_ind];

	if(*spec == 0)
		die(0, "empty argument for tesseract option -l");

	const size_t len = strlen(spec);

	// load list of supported languages
	const str_list* const langs = tess_langs();

	// match each part separated by '+'
	for(const char* lang = spec; lang < spec + len; )
	{
		// next token
		const char* end = strchr(lang, '+');
		const size_t n = end ? (end - lang) : (spec + len - lang);

		if(n == 0)
			die(0, "invalid argument for tesseract option -l: \"%s\"", spec);

		// match
		size_t i = 0;

		for(; i < langs->len; ++i)
		{
			const str l = langs->strings[i];

			if(str_len(l) == n && memcmp(lang, str_ptr(l), n) == 0)
				break;
		}

		if(i == langs->len)
			die(0, "unsupported language in the argument to tesseract option -l: \"%.*s\"", (int)n, lang);

		lang += n + 1;
	}

	// disallow other instances of '-l' option
	for(++opt_ind; opt_ind < num_args; ++opt_ind)
		if(strcmp(args[opt_ind], "-l") == 0)
			die(0, "only one tesseract '-l' option is allowed");
}

int main(int argc, char* argv[])
{
	// command line options
	command cmd;

	parse_options(&cmd, argc, argv);

	// make sure stdin is closed on exec
	just(fcntl(STDIN_FILENO, F_SETFD, fcntl(STDIN_FILENO, F_GETFD) | FD_CLOEXEC));

	// check if tesseract is installed
	tess_check();

	// check language spec
	check_tess_lang_opt(cmd.tess_argv, cmd.tess_argc);

	// get file list
	str_list* const files = list_files(cmd.dir, cmd.spec, "pgm");

	if(str_list_is_empty(files))
	{
		if(cmd.fail_on_empty)
			error(2, 0, "no pages found");

		return 0;
	}

	// run OCR
	for(size_t i = 0; i < files->len; ++i)
	{
		const str file = files->strings[i];
		const unsigned page = page_no(file, str_lit("pgm"));

		info("processing page %u [ \"%s\" ]", page, str_ptr(file));

		tess_extract_text(file, cmd.tess_argv, cmd.tess_argc);
	}

	return 0;
}
