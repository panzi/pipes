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

#define FPIPES_IS_FILE(F) ((F) > FPIPES_TEMP)

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
	else if (inaction == FPIPES_TEMP) {
		pipes->in = tmpfile();

		if (pipes->in == NULL) {
			goto error;
		}

		infd = fileno(pipes->in);
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
	else if (outaction == FPIPES_TO_STDOUT || outaction == FPIPES_TO_STDERR) {
		// see below
		pipes->err = NULL;
	}
	else if (FPIPES_IS_FILE(outaction)) {
		outfd = fileno(pipes->out);
		
		if (outfd < 0) {
			goto error;
		}
		pipes->out = NULL;
	}
	else if (outaction == FPIPES_TEMP) {
		pipes->out = tmpfile();

		if (pipes->out == NULL) {
			goto error;
		}

		outfd = fileno(pipes->out);
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
	else if (erraction == FPIPES_TO_STDOUT || erraction == FPIPES_TO_STDERR) {
		// see below
		pipes->err = NULL;
	}
	else if (FPIPES_IS_FILE(erraction)) {
		errfd = fileno(pipes->err);

		if (errfd < 0) {
			goto error;
		}
		pipes->err = NULL;
	}
	else if (erraction == FPIPES_TEMP) {
		pipes->err = tmpfile();

		if (pipes->err == NULL) {
			goto error;
		}

		errfd = fileno(pipes->err);
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

		// close unused ends
		if (pipes->in != NULL && fileno(pipes->in) != infd) {
			fclose(pipes->in);
			pipes->in = NULL;
		}

		if (pipes->out != NULL && fileno(pipes->out) != outfd) {
			fclose(pipes->out);
			pipes->out = NULL;
		}

		if (pipes->err != NULL && fileno(pipes->err) != errfd) {
			fclose(pipes->err);
			pipes->err = NULL;
		}

		pipes_redirect_fd(infd, STDIN_FILENO, "redirecting stdin");

		if (outaction == FPIPES_TO_STDERR) {
			if (dup2(STDERR_FILENO, STDOUT_FILENO) == -1) {
				perror("redirecting stdout");
				exit(EXIT_FAILURE);
			}
		}
		else {
			pipes_redirect_fd(outfd, STDOUT_FILENO, "redirecting stdout");
		}

		if (erraction == FPIPES_TO_STDOUT) {
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

		if (FPIPES_IS_FILE(inaction)) fclose(inaction);
		else if (inaction  != FPIPES_TEMP && infd  > -1) close(infd);

		if (FPIPES_IS_FILE(outaction)) fclose(outaction);
		else if (outaction != FPIPES_TEMP && outfd > -1) close(outfd);

		if (FPIPES_IS_FILE(erraction)) fclose(erraction);
		else if (erraction != FPIPES_TEMP && errfd > -1) close(errfd);
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
		if (ptr->pipes.in == FPIPES_PIPE && prev->pipes.out != FPIPES_PIPE) {
			errno = EINVAL;
			goto error;
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
		if (fpipes_close(&ptr->pipes) != 0) {
			status = -1;
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

FILE* fpipes_take_in(struct fpipes_chain chain[]) {
	if (chain[0].argv) {
		FILE *fp = chain[0].pipes.in;
		chain[0].pipes.in = NULL;
		return fp;
	}
	else {
		errno = EINVAL;
		return NULL;
	}
}

FILE* fpipes_take_out(struct fpipes_chain chain[]) {
	struct fpipes_chain *prev = chain;
	for (struct fpipes_chain *ptr = chain; ptr->argv; ++ ptr) {
		prev = ptr;
	}

	if (prev->argv) {
		FILE *fp = prev->pipes.out;
		prev->pipes.out = NULL;
		return fp;
	}
	else {
		errno = EINVAL;
		return NULL;
	}
}

FILE* fpipes_take_err(struct fpipes_chain chain[]) {
	struct fpipes_chain *prev = chain;
	for (struct fpipes_chain *ptr = chain; ptr->argv; ++ ptr) {
		prev = ptr;
	}

	if (prev->argv) {
		FILE *fp = prev->pipes.err;
		prev->pipes.err = NULL;
		return fp;
	}
	else {
		errno = EINVAL;
		return NULL;
	}
}
