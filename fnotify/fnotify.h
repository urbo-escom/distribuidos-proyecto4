/*
 * cross-platform library for watching filesystem changes
 */
#ifndef FNOTIFY
#define FNOTIFY


typedef struct fnotify fnotify;


enum fnotify_type {
	FNOTIFY_CREATE = 0x01,
	FNOTIFY_WRITE  = 0x02,
	FNOTIFY_RENAME = 0x04,
	FNOTIFY_DELETE = 0x08,
	FNOTIFY_ISDIR  = 0x10
};


struct fnotify_event {
	enum fnotify_type type;
	char              name[4096];
	char              toname[4096];
};


fnotify* fnotify_open(const char *dirname);
int      fnotify_wait(fnotify *f, struct fnotify_event *e);
int      fnotify_close(fnotify*);


#endif /* !FNOTIFY */
