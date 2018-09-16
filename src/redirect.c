#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

void pipes_redirect_fd(int oldfd, int newfd, const char *msg) {
	if (oldfd > -1) {
		if (oldfd == newfd) {
			// In the (unlikely) case the new file descriptor is the same
			// as the old file descriptor we still need to make sure the
			// close on exec flag is NOT set.
			//
			// This can only happen if the calling process closed all its
			// standard IO streams (thus making them available) and thus
			// the pipe2() call happened to yield the needed target file
			// descriptor just by chance.
			int flags = fcntl(oldfd, F_GETFD);

			if (flags == -1) {
				perror(msg);
				exit(EXIT_FAILURE);
			}

			if ((flags & FD_CLOEXEC) != 0) {
				flags &= ~FD_CLOEXEC;

				if (fcntl(oldfd, F_SETFD, flags) != 0) {
					perror(msg);
					exit(EXIT_FAILURE);
				}
			}
		}
		else {
			if (dup2(oldfd, newfd) == -1) {
				perror(msg);
				exit(EXIT_FAILURE);
			}

			close(oldfd);
		}
	}
}
