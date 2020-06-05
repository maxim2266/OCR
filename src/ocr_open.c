#include "page_spec.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <magic.h>

// usage string
static
const char usage_string[] =
"Usage:\t" PROG_NAME " [OPTION]... FILE\n"
"Renders pages of a .pdf or .djvu FILE to grayscale images in PGM format.\n"
"Options:\n"
"  -p,--pages=SPEC   Pages to extract. A page specification contains one or more comma-separated page\n"
"                    ranges. A page range is either a page number, or two page numbers separated by\n"
"                    a dash. In the last range, the second page number may be omitted, meaning all\n"
"                    the remaining pages of the document. For instance, specification \"1-10\" outputs\n"
"                    pages 1 to 10, and specification \"1,3,5-\" outputs pages 1 and 3, followed by\n"
"                    all the pages starting from page 5 to the end of the document.\n"
"                    (optional, default: all pages)\n"
"  -d,--dir=DIR      Output directory; it must exist. (optional, default: .)\n"
"  -h,--help         Show help and exit.\n"
"  -v,--version      Show version and exit.\n";

// tty flag
static
int _is_tty = 0;

static __attribute__((constructor(50000)))
void _check_tty(void)
{
	_is_tty = isatty(STDOUT_FILENO);
}

#define info(fmt, ...)	\
	if(_is_tty)	\
		printf("%s: " fmt "\n", program_invocation_name, ##__VA_ARGS__)

// directory check
static
const char* check_dir(const char* dir)
{
	struct stat info;

	if(lstat(dir, &info))
		die(errno, "cannot stat \"%s\"", dir);

	if(!S_ISDIR(info.st_mode))
		die(0, "\"%s\" is not a directory", dir);

	if((info.st_mode & S_IWUSR) == 0)
		die(0, "cannot write to \"%s\": permission denied (0%03o)", dir, (unsigned)info.st_mode & 0777);

	return dir;
}

#define format(dest, fmt, ...)	just(asprintf((dest), fmt, ##__VA_ARGS__))

// command line parameters
typedef struct
{
	const char *file, *dir;
	const page_spec* spec;
} command;

// option parser
static
void parse_options(command* const cmd, int argc, char** argv)
{
	// options specification
	static
	const struct option long_options[] =
	{
		{"help",  no_argument, 0, 'h'},
		{"version",  no_argument, 0, 'v'},
		{"pages",  required_argument, 0, 'p'},
		{"dir",  required_argument, 0, 'd'},
		{0, 0, 0, 0}
	};

	// prepare target
	*cmd = (command){0};

	// parser loop
	int opt, option_index = 0;

	while((opt = getopt_long(argc, argv, "+hvp:d:", long_options, &option_index)) >= 0)
	{
		switch(opt)
		{
			case 'h':
				show_usage_and_exit(usage_string);
				break;
			case 'v':
				show_version_and_exit();
				break;
			case 'p':
				if(cmd->spec)
					die(0, "duplicated option: -p, --pages");

				if(!(cmd->spec = parse_page_spec(optarg)))
					die(0, "empty page spec");
				break;
			case 'd':
				if(cmd->dir)
					die(0, "duplicated option: -d, --dir");

				cmd->dir = check_dir(optarg);
				break;
			case '?':
				exit(1);
			default:
				die(0, "internal error (getopt_long(3) returned %d)", opt);
		}
	}

	// option check
	if(!cmd->dir)
		cmd->dir = ".";

	// file
	switch(argc - optind)
	{
		case 0:
			die(0, "input file is not specified");
			break;
		case 1:
			cmd->file = argv[optind];
			break;
		default:
			die(0, "cannot process more than one input file");
			break;
	}
}

// determine file MIME type
static
const char* mime_type(const char* const fname)
{
	magic_t mg = magic_open(MAGIC_SYMLINK
							| MAGIC_ERROR
							| MAGIC_MIME_TYPE
							| MAGIC_PRESERVE_ATIME
							| MAGIC_NO_CHECK_CDF
							| MAGIC_NO_CHECK_COMPRESS
							| MAGIC_NO_CHECK_TAR
							| MAGIC_NO_CHECK_TEXT
							| MAGIC_NO_CHECK_TOKENS);

	if(!mg)
		die(errno, "error opening libmagic");

	if(magic_load(mg, NULL) != 0)
	{
		error(0, 0, "cannot load MIME type database: %s", magic_error(mg));
		magic_close(mg);
		exit(1);
	}

	const char* mime = magic_file(mg, fname);

	if(!mime)
	{
		error(0, 0, "%s", magic_error(mg));
		magic_close(mg);
		exit(1);
	}

	if(!(mime = strdup(mime)))
	{
		error(0, errno, "internal error");
		magic_close(mg);
		exit(1);
	}

	magic_close(mg);

	return mime;
}

// ddjvu
static
void ddjvu(const command* const cmd)
{
	// format for page names
	char* fmt = NULL;

	format(&fmt, "%s/page-%%04d.pgm", cmd->dir);

	info("processing file \"%s\"", cmd->file);

	// exec
	if(!cmd->spec)
	{
		info("extracting all pages");

		just(execlp("ddjvu", program_invocation_name,
					"-format=pgm", "-mode=black", "-eachpage",
					cmd->file, fmt, NULL));
	}
	else
	{
		char* spec;

		format(&spec, "-page=%s", page_spec_to_string(cmd->spec, NULL));

		info("extracting pages %s", spec + sizeof("-page"));

		just(execlp("ddjvu", program_invocation_name,
					"-format=pgm", "-mode=black", "-eachpage",
					spec, cmd->file, fmt, NULL));
	}
}

// get the number of pages in a pdf file, because pdftoppm gives an error
// if the first page requested is past the last page of the document.
static
unsigned pdf_num_pages(const char* const fname)
{
	static const char fmt[] = "pdfinfo \"%s\" 2>/dev/null";

	// prepare script
	char* script = NULL;

	format(&script, fmt, fname);

	// invoke the script
	FILE* const stream = popen(script, "re");

	free(script);

	if(!stream)
		// man popen(3): The popen() function does not set errno if memory allocation fails.
		die((errno == 0) ? ENOMEM : errno,
			"error reading the number of pages in file \"%s\"", fname);

	// read script's output
	char* line = NULL;
	size_t cap = 0;
	ssize_t len;

	int num_pages = -1;

#define PREFIX "Pages:"
#define PREFIX_LEN (sizeof(PREFIX) - 1)

	while((len = getline(&line, &cap, stream)) >= 0)
	{
		if((size_t)len > PREFIX_LEN && memcmp(line, PREFIX, PREFIX_LEN) == 0)
		{
			const char* s = line + PREFIX_LEN;

			// skip whitespace
			while(isspace(*s))
				++s;

			// read the number
			if(*s >= '0' && *s <= '9')
			{
				num_pages = *s++ - '0';

				while(*s >= '0' && *s <= '9')
					num_pages = 10 * num_pages + *s++ - '0';

				// skip remaining whitespace, if any
				while(isspace(*s))
					++s;

				// this must be the end of the string
				if(*s == 0)
				{
					// discard the rest of the stream
					while(getline(&line, &cap, stream) >= 0);
					// all done
					break;
				}

				// reset num_pages and try again
				num_pages = -1;
			}
		}
	}

#undef PREFIX
#undef PREFIX_LEN

	if(line)
		free(line);

	just(pclose(stream));

	if(num_pages < 0)
		die(0, "error reading the number of pages in file \"%s\" (number not found)", fname);

	return (unsigned)num_pages;
}

// run shell script and return it's exit code
static
int shell(const char* const script)
{
	int ret = system(script);

	if(ret != -1)	// -1 only if a child process could not be created
	{
		if(WIFEXITED(ret))			// normal exit
			ret = WEXITSTATUS(ret);
		else if(WIFSIGNALED(ret) && WEXITSTATUS(ret) == 0)	// interrupted by a signal
			ret = 2;
	}

	return ret;
}

// pdftoppm
static
const char* pdftoppm_script(const char* const fname,
							const char* const dir,
							const page_range* const range)
{
	char range_str[64];

	if(range)
		sprintf(range_str, "-f %u -l %u", range->first, range->last);
	else
		range_str[0] = 0;

	static const char script_fmt[] =
		"ERR=\"$(mktemp --tmpdir)\" || exit 1 ; "
		"trap \"rm -f $ERR\" EXIT ; "
		"pdftoppm -gray -scale-to 4000 %s \"%s\" \"%s/page\" 2>\"$ERR\" "
		"|| { grep -vE '^[^:]*\\<[Ww]arning:[[:space:]]+' \"$ERR\" 1>&2 ; exit 1 ; }";

	char* script = NULL;

	format(&script, script_fmt, range_str, fname, dir);

	return script;
}

static __attribute__((noreturn))
void pdftoppm(const command* const cmd)
{
	// format
	char* fmt = NULL;

	format(&fmt, "%s/page", cmd->dir);

	info("processing file \"%s\"", cmd->file);

	if(!cmd->spec)
	{
		// extract all pages in one shot
		info("extracting all pages");

		exit(shell(pdftoppm_script(cmd->file, cmd->dir, NULL)));
	}

	// extract the specified page ranges
	const unsigned num_pages = pdf_num_pages(cmd->file);

	const page_range* p = cmd->spec->ranges;
	const page_range* const end = p + cmd->spec->len;

	int ret = 0;

	for(; ret == 0 && p < end && p->first <= num_pages; ++p)
	{
		const page_range range = (page_range){ p->first, min(p->last, num_pages) };

		info("extracting pages %u-%u", range.first, range.last);

		const char* const script = pdftoppm_script(cmd->file, cmd->dir, &range);

		ret = shell(script);

		free((void*)script);
	}

	free_page_spec(cmd->spec);	// useless...
	exit(ret);
}

// here we start
int main(int argc, char** argv)
{
	if(argc == 1)
		show_usage_and_exit(usage_string);

	command cmd;

	parse_options(&cmd, argc, argv);

	// make sure stdin is closed on exec
	just(fcntl(STDIN_FILENO, F_SETFD, fcntl(STDIN_FILENO, F_GETFD) | FD_CLOEXEC));

	// dispatch on input file MIME type
	const char* const mime = mime_type(cmd.file);

	if(strcmp(mime, "image/vnd.djvu") == 0)
		ddjvu(&cmd);
	else if (strcmp(mime, "application/pdf") == 0)
		pdftoppm(&cmd);
	else
		die(0, "cannot process file \"%s\" of type \"%s\"", cmd.file, mime);

	// must never get here
	abort();
}
