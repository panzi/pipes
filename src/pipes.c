#define _POSIX_SOURCE
#define _GNU_SOURCE

#include "pipes.h"

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef __APPLE__
#	include <crt_externs.h>
#	define environ (*_NSGetEnviron())
#else
	extern char **environ;
#endif

void pipes_redirect_fd(int oldfd, int newfd, const char *msg);

#ifndef P_tmpdir
#	define P_tmpdir "/tmp"
#endif

static int pipes_temp_fd_fallback() {
	char name[] = P_tmpdir "/pipesXXXXXX";
	const int fd = mkstemp(name);

	if (fd < 0) {
		return -1;
	}

	if (unlink(name) != 0) {
		close(fd);
		return -1;
	}

	return fd;
}

#if defined(O_TMPFILE) || defined(__linux__)
#	ifndef O_TMPFILE
#		define O_TMPFILE (020000000 | O_DIRECTORY)
#	endif
static int pipes_temp_fd() {
	int fd = open(P_tmpdir, O_TMPFILE | O_RDWR, S_IRUSR | S_IWUSR);
	if (fd < 0 && (
		errno == EOPNOTSUPP || // no filesystem support for O_TMPFILE
		errno == EISDIR     || // no kernel support for O_TMPFILE
		errno == ENOENT)) {    // no kernel support for O_TMPFILE and path does not exist
		return pipes_temp_fd_fallback();
	}
	return fd;
}
#else
#	define pipes_temp_fd() pipes_temp_fd_fallback()
#endif

int pipes_open(char const *const argv[], char const *const envp[], struct pipes* pipes) {
	int infd  = -1;
	int outfd = -1;
	int errfd = -1;

	const int inaction  = pipes->infd;
	const int outaction = pipes->outfd;
	const int erraction = pipes->errfd;

	// stdin
	if (inaction == PIPES_PIPE) {
		int pair[] = {-1, -1};
		if (pipe(pair) == -1) {
			goto error;
		}

		infd = pair[0];
		pipes->infd = pair[1];
	}
	else if (inaction == PIPES_NULL) {
		infd = open("/dev/null", O_RDONLY);

		if (infd < 0) {
			goto error;
		}
	}
	else if (inaction > -1) {
		infd = inaction;
		pipes->infd = -1;
	}
	else if (inaction == PIPES_TEMP) {
		infd = pipes_temp_fd();
		pipes->infd = infd;

		if (infd < 0) {
			goto error;
		}
	}
	else if (inaction != PIPES_LEAVE) {
		errno = EINVAL;
		goto error;
	}

	// stdout
	if (outaction == PIPES_PIPE) {
		int pair[] = {-1, -1};
		if (pipe(pair) == -1) {
			goto error;
		}

		pipes->outfd = pair[0];
		outfd = pair[1];
	}
	else if (outaction == PIPES_NULL) {
		outfd = open("/dev/null", O_WRONLY);

		if (outfd < 0) {
			goto error;
		}
	}
	else if (outaction == PIPES_TO_STDOUT || outaction == PIPES_TO_STDERR) {
		// see below
	}
	else if (outaction > -1) {
		outfd = outaction;
		pipes->outfd = -1;
	}
	else if (outaction == PIPES_TEMP) {
		outfd = pipes_temp_fd();
		pipes->outfd = outfd;

		if (outfd < 0) {
			goto error;
		}
	}
	else if (outaction != PIPES_LEAVE) {
		errno = EINVAL;
		goto error;
	}

	// stderr
	if (erraction == PIPES_PIPE) {
		int pair[] = {-1, -1};
		if (pipe(pair) == -1) {
			goto error;
		}

		pipes->errfd = pair[0];
		errfd = pair[1];
	}
	else if (erraction == PIPES_NULL) {
		errfd = open("/dev/null", O_WRONLY);

		if (errfd < 0) {
			goto error;
		}
	}
	else if (erraction == PIPES_TO_STDOUT || erraction == PIPES_TO_STDERR) {
		// see below
	}
	else if (erraction > -1) {
		errfd = erraction;
		pipes->errfd = -1;
	}
	else if (erraction == PIPES_TEMP) {
		errfd = pipes_temp_fd();
		pipes->errfd = errfd;

		if (errfd < 0) {
			goto error;
		}
	}
	else if (erraction != PIPES_LEAVE) {
		errno = EINVAL;
		goto error;
	}

	pid_t pid = fork();

	if (pid == -1) {
		goto error;
	}

	if (pid == 0) {
		// child
		pipes_redirect_fd(infd, STDIN_FILENO, "redirecting stdin");

		if (outaction == PIPES_TO_STDERR) {
			if (dup2(STDERR_FILENO, STDOUT_FILENO) == -1) {
				perror("redirecting stdout");
				exit(EXIT_FAILURE);
			}
		}
		else {
			pipes_redirect_fd(outfd, STDOUT_FILENO, "redirecting stdout");
		}

		if (erraction == PIPES_TO_STDOUT) {
			if (dup2(STDOUT_FILENO, STDERR_FILENO) == -1) {
				perror("redirecting stderr");
				exit(EXIT_FAILURE);
			}
		}
		else {
			pipes_redirect_fd(errfd, STDERR_FILENO, "redirecting stderr");
		}

		if (envp) {
			environ = (char**)envp;
		}

		if (execvp(argv[0], (char * const*)argv) == -1) {
			perror(argv[0]);
		}
		exit(EXIT_FAILURE);
	}
	else {
		// parent
		pipes->pid = pid;

		if (inaction  != PIPES_TEMP && infd  > -1) close(infd);
		if (outaction != PIPES_TEMP && outfd > -1) close(outfd);
		if (erraction != PIPES_TEMP && errfd > -1) close(errfd);
	}

	return 0;

error:
	pipes->pid = -1;

	int errnum = errno;

	if (infd  > -1) close(infd);
	if (outfd > -1) close(outfd);
	if (errfd > -1) close(errfd);

	pipes_close(pipes);

	if (errnum != 0) {
		errno = errnum;
	}

	return -1;
}

int pipes_close(struct pipes* pipes) {
	int status = 0;

	if (pipes->infd  > -1) {
		if (close(pipes->infd) != 0) {
			status = -1;
		}
		pipes->infd = -1;
	}

	if (pipes->outfd > -1) {
		if (close(pipes->outfd) != 0) {
			status = -1;
		}
		pipes->outfd = -1;
	}

	if (pipes->errfd > -1) {
		if (close(pipes->errfd) != 0) {
			status = -1;
		}
		pipes->errfd = -1;
	}

	return status;
}

int pipes_open_chain(struct pipes_chain chain[]) {
	struct pipes_chain *ptr  = chain;
	struct pipes_chain *prev = chain;

	for (; ptr->argv; ++ ptr) {
		ptr->pipes.pid = -1;
	}

	if (chain == NULL || chain[0].argv == NULL) {
		errno = EINVAL;
		goto error;
	}

	ptr = chain;

	for (++ ptr; ptr->argv; ++ ptr) {
		if (ptr->pipes.infd == PIPES_PIPE && prev->pipes.outfd != PIPES_PIPE) {
			errno = EINVAL;
			goto error;
		}
		prev = ptr;
	}

	ptr  = chain;
	prev = chain;

	if (pipes_open(ptr->argv, ptr->envp, &ptr->pipes) == -1) {
		goto error;
	}

	for (++ ptr; ptr->argv; ++ ptr) {
		if (ptr->pipes.infd == PIPES_PIPE) {
			ptr->pipes.infd   = prev->pipes.outfd;
			prev->pipes.outfd = -1;
		}

		if (pipes_open(ptr->argv, ptr->envp, &ptr->pipes) == -1) {
			goto error;
		}

		prev = ptr;
	}

	return 0;

error:

	(void)0;

	int errnum = errno;

	pipes_close_chain(chain);
	pipes_kill_chain(chain, SIGTERM);

	if (errnum != 0) {
		errno = errnum;
	}

	return -1;
}

int pipes_close_chain(struct pipes_chain chain[]) {
	int status = 0;

	for (struct pipes_chain *ptr = chain; ptr->argv; ++ ptr) {
		if (pipes_close(&ptr->pipes) != 0) {
			status = -1;
		}
	}

	return status;
}

int pipes_kill_chain(struct pipes_chain chain[], int sig) {
	int status = 0;

	for (struct pipes_chain *ptr = chain; ptr->argv; ++ ptr) {
		if (ptr->pipes.pid > -1) {
			if (kill(ptr->pipes.pid, sig) != 0) {
				status = -1;
			}
		}
	}

	return status;
}

int pipes_take_in(struct pipes_chain chain[]) {
	if (chain[0].argv) {
		int fd = chain[0].pipes.infd;
		chain[0].pipes.infd = -1;
		return fd;
	}
	else {
		errno = EINVAL;
		return -1;
	}
}

int pipes_take_out(struct pipes_chain chain[]) {
	struct pipes_chain *prev = chain;
	for (struct pipes_chain *ptr = chain; ptr->argv; ++ ptr) {
		prev = ptr;
	}

	if (prev->argv) {
		int fd = prev->pipes.outfd;
		prev->pipes.outfd = -1;
		return fd;
	}
	else {
		errno = EINVAL;
		return -1;
	}
}

int pipes_take_err(struct pipes_chain chain[]) {
	struct pipes_chain *prev = chain;
	for (struct pipes_chain *ptr = chain; ptr->argv; ++ ptr) {
		prev = ptr;
	}

	if (prev->argv) {
		int fd = prev->pipes.errfd;
		prev->pipes.errfd = -1;
		return fd;
	}
	else {
		errno = EINVAL;
		return -1;
	}
}
