#include "pipes.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "not enough arguments\n");
		return 1;
	}

	int fd = open(argv[1], O_RDONLY);

	if (fd == -1) {
		perror(argv[1]);
		return 1;
	}

	char const* grep[] = {"grep", "\\w\\+(.*)", NULL};
	char const* sed[]  = {"sed", "s/.*\\b\\(\\w\\+\\)(.*).*/\\1/", NULL};

	struct pipes_chain chain[] = {
		{ PIPES_IN(fd), grep, NULL },
		{ PIPES_PASS,   sed,  NULL },
		{ PIPES_PASS,   NULL, NULL }
	};

	if (pipes_open_chain(chain) == -1) {
		perror("pipes_open_chain");
		return 1;
	}

	char buf[BUFSIZ];

	for (;;) {
		ssize_t size = read(chain[1].pipes.outfd, buf, BUFSIZ);

		if (size == 0) break;
		if (size < 0) {
			perror("read");
			return 1;
		}

		if (fwrite(buf, (size_t)size, 1, stdout) != 1) {
			perror("fwrite");
			return 1;
		}
	}

	int status = 0;
	if (waitpid(chain[1].pipes.pid, &status, 0) == -1) {
		perror("waitpid");
		return 1;
	}

	printf("status of last in chain: %d\n", status);

	pipes_close_chain(chain);

	return 0;
}
