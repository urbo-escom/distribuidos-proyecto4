#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shfs.h"


void* monitor_thread(void *param)
{
	struct shfs *fs = param;

	while (1) {
		struct shfs_file_op op_alloc;
		struct shfs_file_op *op = &op_alloc;
		struct fnotify_event e;
		int s;


		memset(op, 0, sizeof(*op));
		s = fnotify_wait(fs->monitor, &e);
		if (-1 == s)
			break;

		if (e.type & FNOTIFY_ISDIR)
			op->type |= FILE_OP_ISDIR;


		switch (e.type & (0
				| FNOTIFY_CREATE
				| FNOTIFY_WRITE
				| FNOTIFY_DELETE
				| FNOTIFY_RENAME)) {


		case FNOTIFY_CREATE:
			fprintf(stderr, "FNOTIFY create '%s' [%s]\n",
				e.name,
				(e.type & FNOTIFY_ISDIR) ? "dir":"file");
			if (e.type & FNOTIFY_ISDIR) {
				op->type |= FILE_OP_BACKUP;
				strcpy(op->name, e.name);
				queue_enqueue(fs->queue_fs, op);
			}
			break;


		case FNOTIFY_WRITE:
			fprintf(stderr, "FNOTIFY write  '%s' [%s]\n",
				e.name,
				(e.type & FNOTIFY_ISDIR) ? "dir":"file");
			op->type |= FILE_OP_BACKUP;
			strcpy(op->name, e.name);
			queue_enqueue(fs->queue_fs, op);
			break;


		case FNOTIFY_DELETE:
			fprintf(stderr, "FNOTIFY delete '%s'\n",
				e.name);
			op->type |= FILE_OP_DELETE;
			strcpy(op->name, e.name);
			queue_enqueue(fs->queue_fs, op);
			break;


		case FNOTIFY_RENAME:
			fprintf(stderr, "FNOTIFY rename '%s' -> '%s' [%s]\n",
				e.name[0] ? e.name:"UNKNOWN",
				e.toname[0] ? e.toname:"UNKNOWN",
				(e.type & FNOTIFY_ISDIR) ? "dir":"file");
			if (!e.name[0] || !e.toname[0])
				break;
			op->type |= FILE_OP_RENAME;
			strcpy(op->name,   e.name);
			strcpy(op->toname, e.toname);
			queue_enqueue(fs->queue_fs, op);
			break;
		}
	}

	return NULL;
}
