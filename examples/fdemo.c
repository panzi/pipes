#define _POSIX_SOURCE

#include "fpipes.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>

int main(int argc, const char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "usage: %s <filename>\n", argv[0]);
		return 1;
	}

	FILE* fp = fopen(argv[1], "rb");

	if (fp == NULL) {
		perror(argv[1]);
		return 1;
	}

	char const* grep[] = {"grep", "^[^#]*\\w\\+(.*)", NULL};
	char const* sed[]  = {"sed", "s/.*\\b\\(\\w\\+\\)(.*).*/\\1/", NULL};
	char const* sort[] = {"sort", "-u", NULL};

	struct fpipes_chain chain[] = {
		{ FPIPES_IN(fp), grep, NULL },
		{ FPIPES_PASS,   sed,  NULL },
		{ FPIPES_PASS,   sort, NULL },
		{ FPIPES_PASS,   NULL, NULL }
	};

	if (fpipes_open_chain(chain) == -1) {
		perror("fpipes_open_chain");
		return 1;
	}

	char buf[BUFSIZ];

	for (;;) {
		size_t size = fread(buf, 1, BUFSIZ, FPIPES_GET_OUT(chain));

		if (size == 0) {
			if (ferror(FPIPES_GET_OUT(chain))) {
				perror("read");
				fpipes_close_chain(chain);
				return 1;
			}
			break;
		}

		if (fwrite(buf, size, 1, stdout) != 1) {
			perror("fwrite");
			fpipes_close_chain(chain);
			return 1;
		}
	}

	int status = 0;
	if (waitpid(FPIPES_GET_LAST(chain).pid, &status, 0) == -1) {
		perror("waitpid");
		fpipes_close_chain(chain);
		return 1;
	}

	printf("status of last in chain: %d\n", status);

	fpipes_close_chain(chain);

	return 0;
}
