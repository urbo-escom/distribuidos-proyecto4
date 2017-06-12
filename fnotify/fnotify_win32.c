#ifndef UNICODE
#define UNICODE
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <process.h>
#include <shellapi.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "fnotify.h"


/*
 * convert UTF-8 string to UTF-16.
 */
static void *utf8_to_wide(const char *utf8)
{
	WCHAR *str;
	int len;

	/* Get space needed, allocate and convert */
	len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (!len)
		return NULL;

	if ((str = malloc(len*sizeof(char))) == NULL)
		return NULL;

	len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, str, len);
	if (!len)
		free(str), str = NULL;

	return str;
}


/*
 * convert UTF-16 string to UTF-8.
 */
static char *wide_to_utf8(const void *utf16)
{
	const WCHAR *t;
	char *str;
	int len;

	t = utf16;
	/* Get space needed, allocate and convert */
	len = WideCharToMultiByte(CP_UTF8, 0, t, -1, NULL, 0, NULL, NULL);
	if (!len)
		return NULL;

	if ((str = malloc(len*sizeof(char))) == NULL)
		return NULL;

	len = WideCharToMultiByte(CP_UTF8, 0, t, -1, str, len, NULL, NULL);
	if (!len)
		free(str), str = NULL;

	return str;
}


static void perror_win32(const char *s)
{
	DWORD err;
	err = GetLastError();
	if (s != NULL && s[0] != '\0')
		fprintf(stderr, "%s: error %lu(0x%02lx)\n", s, err, err);
	else
		fprintf(stderr, "error %lu(0x%02lx)\n", err, err);
	SetLastError(err);
}


struct fnotify {
	HANDLE                   handle;
	char                    *dirname;
	wchar_t                  buffer[8192];
	FILE_NOTIFY_INFORMATION *info;
};


fnotify* fnotify_open(const char *dirname)
{
	struct fnotify *f = NULL;
	wchar_t *s = NULL;
	char    *d = NULL;


	s = utf8_to_wide(dirname);
	if (NULL == s) {
		perror("utf8_to_wide");
		goto err;
	}


	d = wide_to_utf8(s);
	if (NULL == d) {
		perror("wide_to_utf8");
		goto err;
	}


	f = calloc(1, sizeof(*f));
	if (NULL == f) {
		perror("malloc");
		goto err;
	}


	f->handle = CreateFileW(s, FILE_LIST_DIRECTORY, 0
		| FILE_SHARE_READ
		| FILE_SHARE_WRITE
		| FILE_SHARE_DELETE,
		NULL,
		OPEN_EXISTING, 0
		| FILE_FLAG_BACKUP_SEMANTICS
		| FILE_FLAG_OVERLAPPED,
		NULL
	);
	if (INVALID_HANDLE_VALUE == f->handle) {
		perror_win32("CreateFileW");
		goto err;
	}
	f->dirname = d;


	free(s);
	return f;
err:
	if (NULL != f) free(f);
	if (NULL != s) free(s);
	if (NULL != d) free(d);
	return NULL;
}


int fnotify_close(fnotify *f)
{
	CloseHandle(f->handle);
	free(f->dirname);
	free(f);
	return 0;
}


static int isdir(fnotify *f, wchar_t *name)
{
	wchar_t fullname[8192];
	DWORD attr;

	wsprintfW(fullname, L"%hs/%ls", f->dirname, name);
	attr = GetFileAttributesW(fullname);
	if (INVALID_FILE_ATTRIBUTES == attr)
		return 0;

	return attr & FILE_ATTRIBUTE_DIRECTORY;
}


static void fixslash(char *s)
{
	char *t;
	while (NULL != (t = strchr(s, '\\')))
		*t = '/';
}


static int parse_change(fnotify *f, struct fnotify_event *e)
{
	wchar_t *wname;
	char    *name;

	wname = malloc((f->info->FileNameLength + 1)*sizeof(wchar_t));
	if (NULL == wname) {
		perror("malloc");
		return -1;
	}
	memcpy(wname, f->info->FileName, f->info->FileNameLength);
	wname[f->info->FileNameLength/sizeof(wname[0])] = L'\0';

	name = wide_to_utf8(wname);
	if (NULL == name) {
		perror("malloc");
		free(wname);
		return -1;
	}
	fixslash(name);

	switch (f->info->Action) {
	default:
		fprintf(stderr, "%s: '%s' unknown action %ld(%lx)\n",
			f->dirname,
			name,
			f->info->Action,
			f->info->Action);
		free(wname);
		free(name);
		return -1;

	case FILE_ACTION_ADDED:
		e->type = FNOTIFY_CREATE | (isdir(f, wname) ? FNOTIFY_ISDIR:0);
		strncpy(e->name, name, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		free(wname);
		free(name);
		return 0;

	case FILE_ACTION_REMOVED:
		e->type = FNOTIFY_DELETE;
		strncpy(e->name, name, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		free(wname);
		free(name);
		return 0;

	case FILE_ACTION_MODIFIED:
		/* FIXME: directory write events triggered before deletion */
		e->type = FNOTIFY_WRITE | (isdir(f, wname) ? FNOTIFY_ISDIR:0);
		strncpy(e->name, name, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		free(wname);
		free(name);
		if (e->type & FNOTIFY_ISDIR)
			return 1;
		return 0;

	case FILE_ACTION_RENAMED_OLD_NAME:
		e->type = FNOTIFY_RENAME | (isdir(f, wname) ? FNOTIFY_ISDIR:0);
		strncpy(e->name, name, sizeof(e->name) - 1);
		e->name[sizeof(e->name) - 1] = '\0';
		free(wname);
		free(name);
		return 1;

	case FILE_ACTION_RENAMED_NEW_NAME:
		e->type = FNOTIFY_RENAME | (isdir(f, wname) ? FNOTIFY_ISDIR:0);
		strncpy(e->toname, name, sizeof(e->toname) - 1);
		e->toname[sizeof(e->toname) - 1] = '\0';
		free(wname);
		free(name);
		return 0;
	}

	assert(!"Impossible case");
	return -1;
}


int fnotify_wait(fnotify *f, struct fnotify_event *e)
{
	assert(NULL != f && "f cannot be NULL");

	while (1) {
		DWORD buflen;
		int s;

		if (NULL == f->info) {
			s = ReadDirectoryChangesW(f->handle,
				f->buffer,
				sizeof(f->buffer)/sizeof(f->buffer[0]),
				TRUE, 0
				| FILE_NOTIFY_CHANGE_LAST_WRITE
				| FILE_NOTIFY_CHANGE_CREATION
				| FILE_NOTIFY_CHANGE_FILE_NAME
				| FILE_NOTIFY_CHANGE_DIR_NAME
				,
				&buflen,
				NULL,
				NULL);
			if (0 == s) {
				perror_win32("ReadDirectoryChangesW");
				return -1;
			}
			if (0 == buflen) {
				fprintf(stderr, "%s: buffer was too small\n",
					f->dirname);
				return -1;
			}
			f->info = (FILE_NOTIFY_INFORMATION*)&f->buffer;
		}

		s = parse_change(f, e);
		if (-1 == s)
			return s;

		if (0 == f->info->NextEntryOffset) {
			f->info = NULL;
		} else {
			f->info = (FILE_NOTIFY_INFORMATION*)(
				(char*)f->info + f->info->NextEntryOffset
			);
		}

		if (0 == s)
			return s;
	}

	return 0;
}
