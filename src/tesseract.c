#include "utils.h"
#include "tesseract.h"

#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <assert.h>

#define _die(code, msg, ...) 	(error(0, (code), "" msg, ##__VA_ARGS__), _exit(1))

// wait for child process to terminate
static
void wait_child(const int pid)
{
	int status;

	just(waitpid(pid, &status, 0));
	check_exit_status(status);
}

static __attribute__((noreturn))
void bash_child(const char* const script)
{
	execlp("bash", program_invocation_name, "-c", script, NULL);
	_die(errno, "internal error (execlp)");
}

static
void bash(const char* const script)
{
	const int pid = just(fork());

	if(pid == 0)
		bash_child(script);
	else
		wait_child(pid);
}

// static
// void sh(const char* const script)
// {
// 	check_exit_status(system(script));
// }

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
		int:		_just_int_impl,	\
		default: 	NULL	\
	)((expr), __FILE__, __LINE__)

typedef enum { RD_NONE, RD_STDOUT, RD_STDERR } redirect;

static __attribute__((noreturn))
void read_out_child(const redirect flags,
					const int pfd[2],
					const char* const prog,
					const char* const args[])
{
	_just(close(pfd[0]));				// unused read end

	if(flags & RD_STDOUT)
		_just(dup2(pfd[1], STDOUT_FILENO));	// dup write end to stdout

	if(flags & RD_STDERR)
		_just(dup2(pfd[1], STDERR_FILENO));	// dup write end to stderr

	_just(close(pfd[1]));				// close the write end

	// exec command
	execvp(prog, (char**)args);
	_die(errno, "internal error: execvp(\"%s\")", prog);
}

static
str_list* read_str_list(const int fd)
{
	str_list* list = NULL;

	// open input stream
	FILE* const stream = just(fdopen(fd, "r"));

	// read command output
	char* line = NULL;
	size_t cap = 0;
	ssize_t len;

	while((len = getline(&line, &cap, stream)) >= 0)
	{
		// strip trailing whitespace
		while(len > 0 && isspace(line[len - 1]))
			--len;

		if(len > 0)
			list = str_list_append_copy(list, str_ref_chars(line, len));
	}

	if(line)
		free(line);

	just(fclose(stream));

	return list;
}

typedef struct
{
	str_list* list;
	int status;
} read_out_result;

static
read_out_result _read_out_impl(const redirect flags,
							   const char* const prog,
							   const char* const args[])
{
	assert((flags & (RD_STDOUT | RD_STDERR)) != 0);
	assert(prog && *prog != 0);
	assert(args && *args != NULL);

	// pipe for redirects
	int pfd[2];

	just(pipe(pfd));

	// fork
	const int pid = just(fork());

	if(pid == 0)	// child process
		read_out_child(flags, pfd, prog, args);

	just(close(pfd[1]));	// unused write end

	// read file list
	read_out_result res = (read_out_result){ .list = read_str_list(pfd[0]) };

	// wait for the child to terminate
	just(waitpid(pid, &res.status, 0));

	// check the status
	if(WIFEXITED(res.status))
		res.status = WEXITSTATUS(res.status);
	else if(WIFSIGNALED(res.status))
	{
		const int sig = WTERMSIG(res.status);

		die(0, "program \"%s\" killed by signal %d: %s", prog, sig, strsignal(sig));
	}

	// done
	return res;
}

#define read_out(flags, prog, ...)	\
({	\
	const char* const __args[] = { program_invocation_name, ##__VA_ARGS__, NULL };	\
	_read_out_impl((flags), (prog), __args);	\
})

#define TESS_ERR_PREFIX "Error"
#define TESS_ERR_PREFIX_LEN (sizeof(TESS_ERR_PREFIX) - 1)

static
void exit_if_tess_error(const str line)
{
	const char* s = str_ptr(line);

	if(str_len(line) > TESS_ERR_PREFIX_LEN
		&& memcmp(s, TESS_ERR_PREFIX, TESS_ERR_PREFIX_LEN) == 0
		&& (isspace(s[TESS_ERR_PREFIX_LEN]) || s[TESS_ERR_PREFIX_LEN] == ':'))
	{
		error(255, 0, "\"tesseract\" error: %s", s);
	}
}

#undef TESS_ERR_PREFIX
#undef TESS_ERR_PREFIX_LEN

static
str_list* tess_just(const read_out_result res)
{
	if(res.status == 0)
		return res.list;

	if(!str_list_is_empty(res.list))
		for(size_t i = 0; i < res.list->len; ++i)
			exit_if_tess_error(res.list->strings[i]);

	error(255, 0, "program \"tesseract\" exited with code %d", res.status);
	exit(1);	// unreachable
}

static
void tess_run(const char* args[])
{
	str_list* const list = tess_just(_read_out_impl(RD_STDOUT | RD_STDERR, "tesseract", args));

	str_list_free(list);
}

// check tesseract presence and version
void tess_check(void)
{
	bash
	(
		"set -o pipefail; "
		"die() { echo >&2 \"$0: $*\"; exit 1; }; "
		"VER=\"$(tesseract -v | head -q -n 1 | cut -s -d ' ' -f 2)\" || exit 1; "
		"(( ${VER%%.[0-9]*} >= 4 )) "
		"|| die \"supported 'tesseract' version is 4.0.0 or later, this one is \\\"$VER\\\".\""
	);
}

// read list of installed languages
str_list* tess_langs(void)
{
	str_list* const list = tess_just(read_out(RD_STDOUT | RD_STDERR, "tesseract", "--list-langs"));

	// discard the first line by replacing it with the last one
	if(list->len > 1)
		str_assign(&list->strings[0], str_move(&list->strings[--list->len]));

	return list;
}

static
void check_file(const str file)
{
	const char* const name = str_ptr(file);

	struct stat info;

	if(lstat(name, &info) < 0)
		die(errno, "cannot stat file \"%s\"", name);

	if(!S_ISREG(info.st_mode))
		die(0, "\"%s\" is not a file", name);

	if((info.st_mode & S_IRUSR) == 0)
		die(0, "cannot read \"%s\": permission denied (0%03o)",
			name, (unsigned)info.st_mode & 0777);
}

#define EXT 	".pgm"
#define EXT_LEN	(sizeof(EXT) - 1)

static
void tess_templ(str* const dest, const str name)
{
	// check extension
	if(!str_has_suffix(name, str_lit(EXT)))
		die(0, "unexpected file name \"%.*s\"", (int)str_len(name), str_ptr(name));

	str_cpy(dest, str_ref_chars(str_ptr(name), str_len(name) - EXT_LEN));
}

// extract text from the given file
void tess_extract_text(const str file, const char** opts, const unsigned num_opts)
{
	// check file
	check_file(file);

	// template name
	str templ = str_null;

	tess_templ(&templ, file);

	// args list
	const char** const args = mem_alloc((6 + num_opts) * sizeof(char*));
	const char** p = args;

	*p++ = program_invocation_name;
	*p++ = str_ptr(file);
	*p++ = str_ptr(templ);
	*p++ = "-c";
	*p++ = "page_separator=";

	for(unsigned i = 0; i < num_opts; ++i)
		*p++ = opts[i];

	*p = NULL;

	tess_run(args);

	str_free(templ);
	mem_free(args);
}
