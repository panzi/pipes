#define _POSIX_SOURCE

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

int pipes_open(char const *const argv[], char const *const envp[], struct pipes* pipes) {
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
		goto error;
	}

	if (pid == 0) {
		// child
		redirect_fd(infd,  STDIN_FILENO,  "redirecting stdin");
		redirect_fd(outfd, STDOUT_FILENO, "redirecting stdout");
		redirect_fd(errfd, STDERR_FILENO, "redirecting stderr");

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

		if (infd  > -1) close(infd);
		if (outfd > -1) close(outfd);
		if (errfd > -1) close(errfd);
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
		status = -1;
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
	struct pipes_chain *last = chain;

	for (; ptr->argv; ++ ptr) {
		ptr->pipes.pid = -1;
	}

	if (chain == NULL || chain[0].argv == NULL) {
		errno = EINVAL;
		goto error;
	}

	ptr = chain;

	for (++ ptr; ptr->argv; ++ ptr) {
		if (ptr->pipes.infd == PIPES_PIPE) {
			if (last->pipes.outfd < 0 && last->pipes.outfd != PIPES_PIPE) {
				errno = EINVAL;
				goto error;
			}
		}
		last = ptr;
	}

	ptr  = chain;
	last = chain;

	if (pipes_open(ptr->argv, ptr->envp, &ptr->pipes) == -1) {
		goto error;
	}

	for (++ ptr; ptr->argv; ++ ptr) {
		if (ptr->pipes.infd == PIPES_PIPE) {
			ptr->pipes.infd   = last->pipes.outfd;
			last->pipes.outfd = -1;
		}

		if (pipes_open(ptr->argv, ptr->envp, &ptr->pipes) == -1) {
			goto error;
		}

		last = ptr;
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

	for (struct pipes_chain *ptr = chain; ptr; ++ ptr) {
		if (ptr->pipes.infd > -1) {
			if (close(ptr->pipes.infd) != 0) {
				status = -1;
			}
			ptr->pipes.infd = -1;
		}

		if (ptr->pipes.outfd > -1) {
			if (close(ptr->pipes.outfd) != 0) {
				status = -1;
			}
			ptr->pipes.outfd = -1;
		}

		if (ptr->pipes.errfd > -1) {
			if (close(ptr->pipes.errfd) != 0) {
				status = -1;
			}
			ptr->pipes.errfd = -1;
		}
	}

	return status;
}

int pipes_kill_chain(struct pipes_chain chain[], int sig) {
	int status = 0;

	for (struct pipes_chain *ptr = chain; ptr; ++ ptr) {
		if (ptr->pipes.pid > -1) {
			if (kill(ptr->pipes.pid, sig) != 0) {
				status = -1;
			}
		}
	}

	return status;
}
