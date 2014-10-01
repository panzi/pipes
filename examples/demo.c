#include "pipes.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		return 1;
	}

	int fd = open(argv[1], O_RDONLY);

	if (fd == -1) {
		perror(argv[1]);
		return 1;
	}

	char const* grep[] = {"grep", "^[^#]*\\w\\+(.*)", NULL};
	char const* sed[]  = {"sed", "s/.*\\b\\(\\w\\+\\)(.*).*/\\1/", NULL};
	char const* sort[] = {"sort", "-u", NULL};

	struct pipes_chain chain[] = {
		{ PIPES_IN(fd), grep, NULL },
		{ PIPES_PASS,   sed,  NULL },
		{ PIPES_PASS,   sort, NULL },
		{ PIPES_PASS,   NULL, NULL }
	};

	if (pipes_open_chain(chain) == -1) {
		perror("pipes_open_chain");
		return 1;
	}

	char buf[BUFSIZ];

	for (;;) {
		ssize_t size = read(PIPES_GET_OUT(chain), buf, BUFSIZ);

		if (size == 0) break;
		if (size < 0) {
			perror("read");
			pipes_close_chain(chain);
			return 1;
		}

		if (fwrite(buf, (size_t)size, 1, stdout) != 1) {
			perror("fwrite");
			pipes_close_chain(chain);
			return 1;
		}
	}

	int status = 0;
	if (waitpid(PIPES_GET_LAST(chain).pid, &status, 0) == -1) {
		perror("waitpid");
		pipes_close_chain(chain);
		return 1;
	}

	printf("status of last in chain: %d\n", status);

	pipes_close_chain(chain);

	return 0;
}
