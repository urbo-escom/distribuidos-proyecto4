#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <sys/inotify.h>
#include <unistd.h>

#include "fnotify.h"


struct watch_dir {
	int  fd;
	char name[256];
};


struct watch_list {
	struct watch_dir dirs[128];
	size_t len;
};


struct fnotify {
	int                nfd;
	char              *dirname;
	char               buffer[8192]
		__attribute__((aligned(__alignof__(struct inotify_event))));
	ssize_t            buflen;
	char              *ptr;
	struct watch_list  wl;
	char               lastdir[8192];
};


static struct watch_dir* watch_list_add(struct watch_list *wl, int wfd,
		const char *name)
{
	int fd;
	if (sizeof(wl->dirs)/sizeof(wl->dirs[0]) <= wl->len)
		return NULL;

	fd = inotify_add_watch(wfd, name, 0
		| IN_CREATE
		| IN_MOVE
		| IN_CLOSE_WRITE
		| IN_DELETE
	);
	if (-1 == fd) {
		perror(name);
		return NULL;
	}

	wl->dirs[wl->len].fd = fd;
	strcpy(wl->dirs[wl->len].name, name);
	return &wl->dirs[wl->len++];
}


static struct watch_dir* watch_list_rename(struct watch_list *wl,
		const char *old, const char *new)
{
	size_t i;
	for (i = 0; i < wl->len; i++) {
		if (0 != strcmp(wl->dirs[i].name, old))
			continue;
		strcpy(wl->dirs[i].name, new);
		return &wl->dirs[i];
	}
	return NULL;
}


static void watch_list_remove(struct watch_list *wl,
		int wfd, const char *name)
{
	size_t i;
	size_t j;
	for (i = j = 0; i < wl->len; i++) {
		if (0 != strcmp(wl->dirs[i].name, name)) {
			j++;
			continue;
		}
		inotify_rm_watch(wfd, wl->dirs[i].fd);
		break;
	}
	if (i == j && j == wl->len)
		return;
	for (; i < wl->len; i++, j++)
		memcpy(&wl->dirs[j], &wl->dirs[i], sizeof(wl->dirs[j]));
	wl->len--;
}


static struct watch_dir* watch_list_find_by_fd(struct watch_list *wl,
		int fd)
{
	size_t i;
	for (i = 0; i < wl->len; i++)
		if (wl->dirs[i].fd == fd)
			return &wl->dirs[i];
	return NULL;
}


fnotify* fnotify_open(const char *dirname)
{
	struct fnotify *f = NULL;

	f = calloc(1, sizeof(*f));
	if (NULL == f) {
		perror("malloc");
		return NULL;
	}

	f->dirname = strdup(dirname);
	if (NULL == f->dirname) {
		perror("strdup");
		free(f);
		return NULL;
	}

	f->nfd = inotify_init();
	if (-1 == f->nfd) {
		perror("inotify_init");
		free(f->dirname);
		free(f);
		return NULL;
	}

	if (NULL == watch_list_add(&f->wl, f->nfd, dirname)) {
		close(f->nfd);
		free(f->dirname);
		free(f);
		return NULL;
	}

	return f;
}


int fnotify_close(fnotify *f)
{
	close(f->nfd);
	free(f->dirname);
	free(f);
	return 0;
}


static int parse_change_dir(fnotify *f, struct fnotify_event *e,
		const struct inotify_event *event, const struct watch_dir *dir)
{
	char fname[8192];
	char *filename;

	if (event->len) sprintf(fname, "%s/%s", dir->name, event->name);
	else            sprintf(fname, "%s", dir->name);

	if (!(event->mask & (0
			| IN_CREATE
			| IN_DELETE
			| IN_MOVED_FROM
			| IN_MOVED_TO)))
		return 1;


	if (event->mask & IN_CREATE) {
		e->type = FNOTIFY_CREATE | FNOTIFY_ISDIR;
		filename = fname + strlen(f->dirname) + 1;
		strncpy(e->name, filename, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		return NULL == watch_list_add(&f->wl, f->nfd, fname) ? -1:0;
	}


	if (event->mask & (IN_DELETE | IN_IGNORED)) {
		e->type = FNOTIFY_DELETE;
		filename = fname + strlen(f->dirname) + 1;
		strncpy(e->name, filename, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		return watch_list_remove(&f->wl, f->nfd, fname), 0;
	}


	if (event->mask & IN_MOVED_FROM)
		return strcpy(f->lastdir, fname), 1;


	if (event->mask & IN_MOVED_TO) {
		e->type = FNOTIFY_RENAME | FNOTIFY_ISDIR;

		filename = f->lastdir + strlen(f->dirname) + 1;
		strncpy(e->name, filename, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';

		filename = fname + strlen(f->dirname) + 1;
		strncpy(e->toname, filename, sizeof(e->toname) - 1);
		e->toname[sizeof(e->toname) - 1] = '\0';

		watch_list_rename(&f->wl, f->lastdir, fname);
		return 0;
	}


	return 1;
}


static int parse_change_file(fnotify *f, struct fnotify_event *e,
		const struct inotify_event *event, const struct watch_dir *dir)
{
	char  fname[8192];
	char *filename;

	if (!event->len) {
		fprintf(stderr, "%s: %s file has empty name\n",
			f->dirname, dir->name);
		return -1;
	}
	sprintf(fname, "%s/%s", dir->name, event->name);
	filename = fname + strlen(f->dirname) + 1;


	if (event->mask & IN_CREATE) {
		e->type = FNOTIFY_CREATE;
		strncpy(e->name, filename, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		return 0;
	}


	if (event->mask & IN_DELETE) {
		e->type = FNOTIFY_DELETE;
		strncpy(e->name, filename, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		return 0;
	}


	if (event->mask & IN_CLOSE_WRITE) {
		e->type = FNOTIFY_WRITE;
		strncpy(e->name, filename, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		return 0;
	}


	if (event->mask & IN_MOVED_FROM) {
		e->type = FNOTIFY_RENAME;
		strncpy(e->name, filename, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		return 1;
	}


	if (event->mask & IN_MOVED_TO) {
		e->type = FNOTIFY_RENAME;
		strncpy(e->toname, filename, sizeof(e->toname) - 1);
		e->toname[sizeof(e->toname) - 1] = '\0';
		return 0;
	}


	fprintf(stderr, "%s: %s unknown mask %x\n",
		f->dirname, filename, event->mask);
	return -1;
}


static int parse_change(fnotify *f, struct fnotify_event *e)
{
	const struct inotify_event *event;
	struct watch_dir *dir;


	event = (const struct inotify_event*)f->ptr;
	dir = watch_list_find_by_fd(&f->wl, event->wd);
	if (NULL == dir) {
		fprintf(stderr, "%s: could not find fd %d\n",
			f->dirname, event->wd);
		return -1;
	}


	if (event->mask & (IN_ISDIR | IN_IGNORED))
		return parse_change_dir(f, e, event, dir);
	return parse_change_file(f, e, event, dir);
}


int fnotify_wait(fnotify *f, struct fnotify_event *e)
{
	assert(NULL != f && "f cannot be NULL");

	while (1) {
		int s;

		if (NULL == f->ptr) {
			f->buflen = read(f->nfd, f->buffer, sizeof(f->buffer));
			if (-1 == f->buflen) {
				perror("read");
				f->buflen = 0;
				return -1;
			}
			if (0 == f->buflen)
				continue;
			f->ptr = f->buffer;
		}

		s = parse_change(f, e);
		if (-1 == s)
			return s;

		f->ptr +=
			sizeof(struct inotify_event) +
			((struct inotify_event*)f->ptr)->len;

		if (f->buffer + f->buflen <= f->ptr)
			f->ptr = NULL, f->buflen = 0;

		if (0 == s)
			return s;
	}

	return 0;
}
