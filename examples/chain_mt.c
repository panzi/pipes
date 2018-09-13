#include "pipes.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>

void *thread_func(void *ptr) {
	const char *filename = (const char *)ptr;

	int fd = open(filename, O_RDONLY);

	if (fd == -1) {
		perror(filename);
		exit(EXIT_FAILURE);
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
		exit(EXIT_FAILURE);
	}

	char buf[BUFSIZ];

	for (;;) {
		ssize_t size = read(PIPES_GET_OUT(chain), buf, BUFSIZ);

		if (size == 0) break;
		if (size < 0) {
			perror("read");
			pipes_close_chain(chain);
			exit(EXIT_FAILURE);
		}

		if (fwrite(buf, (size_t)size, 1, stdout) != 1) {
			perror("fwrite");
			pipes_close_chain(chain);
			exit(EXIT_FAILURE);
		}
	}

	int status = 0;
	if (waitpid(PIPES_GET_LAST(chain).pid, &status, 0) == -1) {
		perror("waitpid");
		pipes_close_chain(chain);
		exit(EXIT_FAILURE);
	}

	printf("status of last in chain: %d\n", status);

	pipes_close_chain(chain);

	return NULL;
}

int main(int argc, const char* argv[]) {
	if (argc < 3) {
		fprintf(stderr, "usage: %s <thread_count> <filename>\n", argc < 1 ? "chain_mt" : argv[0]);
		return 1;
	}

	char *endptr = NULL;
	long int count = strtol(argv[1], &endptr, 10);
	if (!*argv[1] || *endptr) {
		perror("parsing thread count");
		return 1;
	}

	if (count < 1) {
		fprintf(stderr, "illegal thread count: %ld\n", count);
		return 1;
	}

	const char *filename = argv[2];

	pthread_t *threads = calloc(sizeof(pthread_t), count);
	if (threads == NULL) {
		perror("creating thread array");
		return 1;
	}

	for (long int i = 0; i < count; ++ i) {
		int errnum = pthread_create(&threads[i], NULL, thread_func, (void*)filename);

		if (errnum != 0) {
			fprintf(stderr, "error creating thread: %s\n", strerror(errnum));
			exit(EXIT_FAILURE);
		}
	}

	for (long int i = 0; i < count; ++ i) {
		int errnum = pthread_join(threads[i], NULL);

		if (errnum != 0) {
			fprintf(stderr, "error joining thread: %s\n", strerror(errnum));
		}
	}

	free(threads);

	return 0;
}
