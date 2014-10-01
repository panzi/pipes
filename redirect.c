#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void pipes_redirect_fd(int oldfd, int newfd, const char *msg) {
	if (oldfd > -1 && oldfd != newfd) {
		close(newfd);

		if (dup2(oldfd, newfd) == -1) {
			perror(msg);
			exit(EXIT_FAILURE);
		}

		close(oldfd);
	}
}
