#include "fpipes.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

int main() {	
	char const* head[] = {"head", "-c", "128", "/dev/random", NULL};
	char const* xxd[]  = {"xxd", NULL};

	struct fpipes_chain chain[] = {
		{ FPIPES_FIRST,            head, NULL },
		{ FPIPES_OUT(FPIPES_TEMP), xxd,  NULL },
		{ FPIPES_PASS,             NULL, NULL }
	};

	if (fpipes_open_chain(chain) == -1) {
		perror("pipes_open_chain");
		return 1;
	}

	int status = 0;
	if (waitpid(FPIPES_GET_LAST(chain).pid, &status, 0) == -1) {
		perror("waitpid");
		fpipes_close_chain(chain);
		return 1;
	}

	FILE *fp = fpipes_take_out(chain);

	fpipes_close_chain(chain);

	fseek(fp, 0, SEEK_SET);

	char buf[BUFSIZ];

	for (;;) {
		size_t size = fread(buf, 1, BUFSIZ, fp);

		if (size == 0) {
			if (ferror(fp)) {
				perror("read");
				fclose(fp);
				return 1;
			}
			break;
		}

		if (fwrite(buf, size, 1, stdout) != 1) {
			perror("fwrite");
			fclose(fp);
			return 1;
		}
	}

	printf("status of last in chain: %d\n", status);

	fclose(fp);

	return 0;
}
