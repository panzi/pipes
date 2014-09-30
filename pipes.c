#define _POSIX_SOURCE

#include "pipes.h"

#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

static void redirect_fd(int from_fd, int to_fd, const char *msg) {
	if (from_fd > -1 && from_fd != to_fd) {
		close(to_fd);

		if (dup2(from_fd, to_fd) == -1) {
			perror(msg);
			exit(EXIT_FAILURE);
		}

		close(from_fd);
	}
}

int pipes_open(char const *const argv[], struct pipes* pipes) {
	int infd  = -1;
	int outfd = -1;
	int errfd = -1;

	int inaction  = pipes->infd;
	int outaction = pipes->outfd;
	int erraction = pipes->errfd;

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
	else if (inaction != PIPES_LEAVE) {
		errno = EINVAL;
		goto error;
	}
	
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
	else if (outaction > -1) {
		outfd = outaction;
		pipes->outfd = -1;
	}
	else if (outaction != PIPES_LEAVE) {
		errno = EINVAL;
		goto error;
	}
	
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
	else if (erraction == PIPES_ERR_TO_OUT) {
		if (outaction == PIPES_LEAVE) {
			errfd = dup(STDOUT_FILENO);
		}
		else {
			errfd = dup(outfd);
		}

		if (errfd < 0) {
			goto error;
		}
	}
	else if (erraction > -1) {
		errfd = erraction;
		pipes->errfd = -1;
	}
	else if (erraction != PIPES_LEAVE) {
		errno = EINVAL;
		goto error;
	}

	pid_t pid = fork();

	if (pid == -1) {
		if (infd > 0) close(infd);
		return -1;
	}

	if (pid == 0) {
		// child
		redirect_fd(infd,  STDIN_FILENO,  "redirecting stdin");
		redirect_fd(outfd, STDOUT_FILENO, "redirecting stdout");
		redirect_fd(errfd, STDERR_FILENO, "redirecting stderr");

		if (execvp(argv[0], (char * const*)argv) == -1) {
			perror(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	else {
		// parent
		pipes->pid = pid;

		if (infd  > -1) close(infd);
		if (outfd > -1) close(outfd);
		if (errfd > -1) close(errfd);
	}

	return 0;

error:
	pipes->pid = -1;

	if (infd  > -1) close(infd);
	if (outfd > -1) close(outfd);
	if (errfd > -1) close(errfd);

	if (pipes->infd  > -1) {
		close(pipes->infd);
		pipes->infd = -1;
	}

	if (pipes->outfd > -1) {
		close(pipes->outfd);
		pipes->outfd = -1;
	}

	if (pipes->errfd > -1) {
		close(pipes->errfd);
		pipes->errfd = -1;
	}

	return -1;
}

int pipes_open_chain(struct pipes_chain chain[]) {
	if (chain == NULL || chain[0].argv == NULL) {
		errno = EINVAL;
		return -1;
	}

	struct pipes_chain *ptr = chain;

	if (chain[1].argv != NULL) {
		ptr->pipes.outfd = PIPES_PIPE;
	}

	if (pipes_open(ptr->argv, &ptr->pipes) == -1) {
		goto error;
	}

	struct pipes_chain *last = ptr;

	for (++ ptr; ptr->argv; ++ ptr) {
		ptr->pipes.infd   = last->pipes.outfd;
		last->pipes.outfd = -1;

		if (pipes_open(ptr->argv, &ptr->pipes) == -1) {
			goto error;
		}

		last = ptr;
	}

	return 0;

error:

	for (ptr = chain; ptr; ++ ptr) {
		if (ptr->pipes.infd > -1) {
			close(ptr->pipes.infd);
			ptr->pipes.infd = -1;
		}

		if (ptr->pipes.outfd > -1) {
			close(ptr->pipes.outfd);
			ptr->pipes.outfd = -1;
		}

		if (ptr->pipes.errfd > -1) {
			close(ptr->pipes.errfd);
			ptr->pipes.errfd = -1;
		}

		if (ptr->pipes.pid > -1) {
			kill(ptr->pipes.pid, SIGTERM);
		}
	}

	return -1;
}
