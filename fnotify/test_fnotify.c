#include <stdio.h>
#include <stdlib.h>

#include "fnotify.h"


int main(int argc, char **argv)
{
	struct fnotify_event e;
	fnotify *f;

	if (argc < 2) {
		fprintf(stderr, "usage: %s DIR\n", argv[0]);
		return EXIT_FAILURE;
	}

	f = fnotify_open(argv[1]);
	while (1) {
		int s;
		s = fnotify_wait(f, &e);
		if (-1 == s)
			break;

		switch (e.type & (0
				| FNOTIFY_CREATE
				| FNOTIFY_WRITE
				| FNOTIFY_DELETE
				| FNOTIFY_RENAME)) {
		case FNOTIFY_CREATE:
			printf("%s: create '%s' [%s]\n",
				argv[1],
				e.name,
				(e.type & FNOTIFY_ISDIR) ? "dir":"file");
			break;

		case FNOTIFY_WRITE:
			printf("%s: write  '%s' [%s]\n",
				argv[1],
				e.name,
				(e.type & FNOTIFY_ISDIR) ? "dir":"file");
			break;

		case FNOTIFY_DELETE:
			printf("%s: delete '%s'\n",
				argv[1],
				e.name);
			break;

		case FNOTIFY_RENAME:
			printf("%s: rename '%s' -> '%s' [%s]\n",
				argv[1],
				e.name[0] ? e.name:"UNKNOWN",
				e.toname[0] ? e.toname:"UNKNOWN",
				(e.type & FNOTIFY_ISDIR) ? "dir":"file");
			break;
		}
	}
	fnotify_close(f);

	return EXIT_SUCCESS;
}
