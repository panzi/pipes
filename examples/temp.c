#include "pipes.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

int main() {	
	char const* head[] = {"head", "-c", "128", "/dev/random", NULL};
	char const* xxd[]  = {"xxd", NULL};

	struct pipes_chain chain[] = {
		{ PIPES_FIRST,           head, NULL },
		{ PIPES_OUT(PIPES_TEMP), xxd,  NULL },
		{ PIPES_PASS,            NULL, NULL }
	};

	if (pipes_open_chain(chain) == -1) {
		perror("pipes_open_chain");
		return 1;
	}

	int status = 0;
	if (waitpid(PIPES_GET_LAST(chain).pid, &status, 0) == -1) {
		perror("waitpid");
		pipes_close_chain(chain);
		return 1;
	}

	int fd = pipes_take_out(chain);

	pipes_close_chain(chain);

	lseek(fd, 0, SEEK_SET);

	char buf[BUFSIZ];

	for (;;) {
		ssize_t size = read(fd, buf, BUFSIZ);

		if (size == 0) break;
		if (size < 0) {
			perror("read");
			close(fd);
			return 1;
		}

		if (fwrite(buf, (size_t)size, 1, stdout) != 1) {
			perror("fwrite");
			close(fd);
			return 1;
		}
	}

	printf("status of last in chain: %d\n", status);

	close(fd);

	return 0;
}
