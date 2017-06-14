#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include <sys/stat.h>

#include "shfs.h"


int fd_list_open(struct shfs *fs, const char *name, size_t length)
{
	char tmp_fname[1024];
	char main_fname[1024];
	size_t i;

	pthread_mutex_lock(&fs->fdlock);

	if (fs->fdsize <= fs->fdlen) {
		fprintf(stderr, "fd_list_open(%s): full\n", name);
		pthread_mutex_unlock(&fs->fdlock);
		return -1;
	}

	sprintf(tmp_fname, "%s/%s", fs->tmpdir, name);
	sprintf(main_fname, "%s/%s", fs->maindir, name);

	for (i = 0; i < fs->fdlen; i++)
		if (0 == strcmp(fs->fdlist[i].name, name)) {
			pthread_mutex_unlock(&fs->fdlock);
			return -1;
		}

	strcpy(fs->fdlist[i].name, name);
	fs->fdlist[i].fd_tmp = fopen(tmp_fname, "wb");
	fs->fdlist[i].fd_main = fopen(main_fname, "wb");
	fs->fdlist[i].offset = 0;
	fs->fdlist[i].length = length;
	fs->fdlist[i].lastrecv = time(NULL);
	fs->fdlen++;

	pthread_mutex_unlock(&fs->fdlock);
	return 0;
}


int fd_list_write(struct shfs *fs, const char *name,
		size_t offset, const void *buf, size_t len)
{
	size_t i;

	fprintf(stderr, "writing to fdlist\n");
	pthread_mutex_lock(&fs->fdlock);
	for (i = 0; i < fs->fdlen; i++) {
		if (0 != strcmp(fs->fdlist[i].name, name))
			continue;
		if (fs->fdlist[i].offset != offset) {
			fprintf(stderr, "Rejecting offset %d\n", (int)offset);
			continue;
		}

		if (NULL != fs->fdlist[i].fd_tmp)
			fwrite(buf, 1, len, fs->fdlist[i].fd_tmp);

		if (NULL != fs->fdlist[i].fd_main)
			fwrite(buf, 1, len, fs->fdlist[i].fd_main);

		fprintf(stderr, "FD_LIST offset+len %d+%d\n",
			(int)offset, (int)len);
		fs->fdlist[i].offset += len;
		fs->fdlist[i].lastrecv = time(NULL);
		pthread_mutex_unlock(&fs->fdlock);
		return 0;
	}
	pthread_mutex_unlock(&fs->fdlock);
	fprintf(stderr, "finish writing to fdlist\n");
	return 0;
}


int fd_list_close(struct shfs *fs, const char *name)
{
	size_t i;

	pthread_mutex_lock(&fs->fdlock);
	for (i = 0; i < fs->fdlen; i++) {
		if (0 != strcmp(fs->fdlist[i].name, name))
			continue;
		fclose(fs->fdlist[i].fd_tmp);
		fclose(fs->fdlist[i].fd_main);
		fs->fdlist[i].fd_tmp = NULL;
		fs->fdlist[i].fd_main = NULL;
		pthread_mutex_unlock(&fs->fdlock);
		return 0;
	}
	pthread_mutex_unlock(&fs->fdlock);
	return -1;
}


int fd_list_has(struct shfs *fs, const char *name)
{
	size_t i;

	pthread_mutex_lock(&fs->fdlock);
	for (i = 0; i < fs->fdlen; i++) {
		if (0 != strcmp(fs->fdlist[i].name, name))
			continue;
		pthread_mutex_unlock(&fs->fdlock);
		return 1;
	}
	pthread_mutex_unlock(&fs->fdlock);
	return 0;
}


int fd_list_remove(struct shfs *fs, const char *name)
{
	size_t i;
	size_t j;

	pthread_mutex_lock(&fs->fdlock);
	for (i = j = 0; i < fs->fdlen; i++) {
		if (0 != strcmp(fs->fdlist[i].name, name)) {
			j++;
			continue;
		}
		break;
	}
	if (i == fs->fdlen)
		return -1;
	for (j = i + 1; j < fs->fdlen; i++, j++)
		memcpy(&fs->fdlist[i], &fs->fdlist[j], sizeof(fs->fdlist[i]));
	fs->fdlen--;
	pthread_mutex_unlock(&fs->fdlock);
	return 0;
}
