#define _POSIX_SOURCE

#include "fpipes.h"

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

#define FPIPES_IS_FILE(F) ((F) > FPIPES_ERR_TO_OUT)

void pipes_redirect_fd(int oldfd, int newfd, const char *msg);

int fpipes_open(char const *const argv[], char const *const envp[], struct fpipes* pipes) {
	int infd  = -1;
	int outfd = -1;
	int errfd = -1;

	FILE* inaction  = pipes->in;
	FILE* outaction = pipes->out;
	FILE* erraction = pipes->err;

	// stdin
	if (inaction == FPIPES_PIPE) {
		int pair[] = {-1, -1};
		if (pipe(pair) == -1) {
			goto error;
		}

		infd = pair[0];
		pipes->in = fdopen(pair[1], "w");

		if (pipes->in == NULL) {
			close(pair[1]);
			goto error;
		}
	}
	else if (inaction == FPIPES_NULL) {
		infd = open("/dev/null", O_RDONLY);
		pipes->in = NULL;

		if (infd < 0) {
			goto error;
		}
	}
	else if (FPIPES_IS_FILE(inaction)) {
		infd = fileno(pipes->in);

		if (infd < 0) {
			goto error;
		}
		pipes->in = NULL;
	}
	else if (inaction == FPIPES_LEAVE) {
		pipes->in = NULL;
	}
	else {
		errno = EINVAL;
		goto error;
	}

	// stdout
	if (outaction == FPIPES_PIPE) {
		int pair[] = {-1, -1};
		if (pipe(pair) == -1) {
			goto error;
		}

		pipes->out = fdopen(pair[0], "r");
		outfd = pair[1];

		if (pipes->out == NULL) {
			close(pair[0]);
			goto error;
		}
	}
	else if (outaction == FPIPES_NULL) {
		outfd = open("/dev/null", O_WRONLY);
		pipes->out = NULL;

		if (outfd < 0) {
			goto error;
		}
	}
	else if (FPIPES_IS_FILE(outaction)) {
		outfd = fileno(pipes->out);
		
		if (outfd < 0) {
			goto error;
		}
		pipes->out = NULL;
	}
	else if (outaction == FPIPES_LEAVE) {
		pipes->out = NULL;
	}
	else {
		errno = EINVAL;
		goto error;
	}

	// stderr
	if (erraction == FPIPES_PIPE) {
		int pair[] = {-1, -1};
		if (pipe(pair) == -1) {
			goto error;
		}

		pipes->err = fdopen(pair[0], "r");
		errfd = pair[1];

		if (pipes->err == NULL) {
			close(pair[0]);
			goto error;
		}
	}
	else if (erraction == FPIPES_NULL) {
		errfd = open("/dev/null", O_WRONLY);
		pipes->err = NULL;

		if (errfd < 0) {
			goto error;
		}
	}
	else if (erraction == FPIPES_ERR_TO_OUT) {
		pipes->err = NULL;

		if (outaction == FPIPES_LEAVE) {
			errfd = dup(STDOUT_FILENO);
		}
		else {
			errfd = dup(outfd);
		}

		if (errfd < 0) {
			goto error;
		}
	}
	else if (FPIPES_IS_FILE(erraction)) {
		errfd = fileno(pipes->err);

		if (errfd < 0) {
			goto error;
		}
		pipes->err = NULL;
	}
	else if (erraction == FPIPES_LEAVE) {
		pipes->err = NULL;
	}
	else {
		errno = EINVAL;
		goto error;
	}

	pid_t pid = fork();

	if (pid == -1) {
		goto error;
	}

	if (pid == 0) {
		// child
		pipes_redirect_fd(infd,  STDIN_FILENO,  "redirecting stdin");
		pipes_redirect_fd(outfd, STDOUT_FILENO, "redirecting stdout");
		pipes_redirect_fd(errfd, STDERR_FILENO, "redirecting stderr");

		if (envp) {
			environ = (char**)envp;
		}

		if (execvp(argv[0], (char * const*)argv) == -1) {
			perror(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	else {
		// parent
		pipes->pid = pid;

		if (FPIPES_IS_FILE(inaction)) fclose(inaction);
		else if (infd  > -1) close(infd);

		if (FPIPES_IS_FILE(outaction)) fclose(outaction);
		else if (outfd > -1) close(outfd);

		if (FPIPES_IS_FILE(erraction)) fclose(erraction);
		else if (errfd > -1) close(errfd);
	}

	return 0;

error:
	pipes->pid = -1;

	int errnum = errno;

	if (infd  > -1) close(infd);
	if (outfd > -1) close(outfd);
	if (errfd > -1) close(errfd);

	if (FPIPES_IS_FILE(inaction)) {
		fclose(inaction);
	}
	else if (pipes->in) {
		fclose(pipes->in);
	}
	pipes->in = NULL;

	if (FPIPES_IS_FILE(outaction)) {
		fclose(outaction);
	}
	else if (pipes->out) {
		fclose(pipes->out);
	}
	pipes->out = NULL;

	if (FPIPES_IS_FILE(erraction)) {
		fclose(erraction);
	}
	else if (pipes->err) {
		fclose(pipes->err);
	}
	pipes->err = NULL;

	if (errnum != 0) {
		errno = errnum;
	}

	return -1;
}


int fpipes_close(struct fpipes* pipes) {
	int status = 0;

	if (FPIPES_IS_FILE(pipes->in)) {
		if (fclose(pipes->in) != 0) {
			status = -1;
		}
		pipes->in = NULL;
	}

	if (FPIPES_IS_FILE(pipes->out)) {
		if (fclose(pipes->out) != 0) {
			status = -1;
		}
		pipes->out = NULL;
	}

	if (FPIPES_IS_FILE(pipes->err)) {
		if (fclose(pipes->err) != 0) {
			status = -1;
		}
		pipes->err = NULL;
	}

	return status;
}

int fpipes_open_chain(struct fpipes_chain chain[]) {
	struct fpipes_chain *ptr  = chain;
	struct fpipes_chain *prev = chain;

	for (; ptr->argv; ++ ptr) {
		ptr->pipes.pid = -1;
	}

	if (chain == NULL || chain[0].argv == NULL) {
		errno = EINVAL;
		goto error;
	}

	ptr = chain;

	for (++ ptr; ptr->argv; ++ ptr) {
		if (ptr->pipes.in == FPIPES_PIPE) {
			if (!FPIPES_IS_FILE(prev->pipes.out) && prev->pipes.out != FPIPES_PIPE) {
				errno = EINVAL;
				goto error;
			}
		}
		prev = ptr;
	}

	ptr  = chain;
	prev = chain;

	if (fpipes_open(ptr->argv, ptr->envp, &ptr->pipes) == -1) {
		goto error;
	}

	for (++ ptr; ptr->argv; ++ ptr) {
		if (ptr->pipes.in == FPIPES_PIPE) {
			ptr->pipes.in   = prev->pipes.out;
			prev->pipes.out = NULL;
		}

		if (fpipes_open(ptr->argv, ptr->envp, &ptr->pipes) == -1) {
			goto error;
		}

		prev = ptr;
	}

	return 0;

error:

	(void)0;

	int errnum = errno;

	fpipes_close_chain(chain);
	fpipes_kill_chain(chain, SIGTERM);

	if (errnum != 0) {
		errno = errnum;
	}

	return -1;
}

int fpipes_close_chain(struct fpipes_chain chain[]) {
	int status = 0;

	for (struct fpipes_chain *ptr = chain; ptr->argv; ++ ptr) {
		if (ptr->pipes.in) {
			if (fclose(ptr->pipes.in) != 0) {
				status = -1;
			}
			ptr->pipes.in = NULL;
		}

		if (ptr->pipes.out) {
			if (fclose(ptr->pipes.out) != 0) {
				status = -1;
			}
			ptr->pipes.out = NULL;
		}

		if (ptr->pipes.err) {
			if (fclose(ptr->pipes.err) != 0) {
				status = -1;
			}
			ptr->pipes.err = NULL;
		}
	}

	return status;
}

int fpipes_kill_chain(struct fpipes_chain chain[], int sig) {
	int status = 0;

	for (struct fpipes_chain *ptr = chain; ptr->argv; ++ ptr) {
		if (ptr->pipes.pid > -1) {
			if (kill(ptr->pipes.pid, sig) != 0) {
				status = -1;
			}
		}
	}

	return status;
}
