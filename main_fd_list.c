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

	fprintf(stderr, "FD_LIST opening %d bytes '%s'\n", (int)length, name);
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
			fprintf(stderr, "FD_LIST already open '%s'\n", name);
			pthread_mutex_unlock(&fs->fdlock);
			return -1;
		}

	memset(&fs->fdlist[i], 0, sizeof(fs->fdlist[i]));
	strcpy(fs->fdlist[i].name, name);
	fs->fdlist[i].offset = 0;
	fs->fdlist[i].length = length;
	fs->fdlist[i].lastrecv = time(NULL);

	fs->fdlist[i].fd_tmp = fopen(tmp_fname, "wb");
	if (NULL == fs->fdlist[i].fd_tmp) {
		fprintf(stderr, "FD_LIST could not open '%s'\n", tmp_fname);
		if (NULL != fs->fdlist[i].fd_tmp)
			fclose(fs->fdlist[i].fd_tmp);
		if (NULL != fs->fdlist[i].fd_main)
			fclose(fs->fdlist[i].fd_main);
		pthread_mutex_unlock(&fs->fdlock);
		return -1;
	}

	fs->fdlist[i].fd_main = fopen(main_fname, "wb");
	if (NULL == fs->fdlist[i].fd_main) {
		fprintf(stderr, "FD_LIST could not open '%s'\n", main_fname);
		if (NULL != fs->fdlist[i].fd_tmp)
			fclose(fs->fdlist[i].fd_tmp);
		if (NULL != fs->fdlist[i].fd_main)
			fclose(fs->fdlist[i].fd_main);
		pthread_mutex_unlock(&fs->fdlock);
		return -1;
	}

	fs->fdlen++;

	pthread_mutex_unlock(&fs->fdlock);
	return 0;
}


int fd_list_write(struct shfs *fs, const char *name,
		size_t offset, const void *buf, size_t len)
{
	size_t i;

	fprintf(stderr, "FD_LIST writing\n");
	pthread_mutex_lock(&fs->fdlock);
	for (i = 0; i < fs->fdlen; i++) {
		if (0 != strcmp(fs->fdlist[i].name, name))
			continue;
		if (fs->fdlist[i].offset != offset) {
			fprintf(stderr, "FD_LIST Bad offset %d vs [%d]@'%s'\n",
				(int)offset, (int)fs->fdlist[i].offset, name);
			continue;
		}
		if (fs->fdlist[i].length <= offset) {
			fprintf(stderr, "FD_LIST offset %d/%d overflow '%s'\n",
				(int)offset, (int)fs->fdlist[i].length, name);
			continue;
		}

		if (NULL != fs->fdlist[i].fd_tmp)
			fwrite(buf, 1, len, fs->fdlist[i].fd_tmp);

		if (NULL != fs->fdlist[i].fd_main)
			fwrite(buf, 1, len, fs->fdlist[i].fd_main);

		fprintf(stderr, "FD_LIST offset+len %d += %d '%s'\n",
			(int)offset, (int)len, name);
		fs->fdlist[i].offset += len;
		fs->fdlist[i].lastrecv = time(NULL);
		pthread_mutex_unlock(&fs->fdlock);
		return 0;
	}
	pthread_mutex_unlock(&fs->fdlock);
	fprintf(stderr, "FD_LIST writing finish\n");
	return 0;
}


int fd_list_close(struct shfs *fs, const char *name)
{
	size_t i;

	fprintf(stderr, "FD_LIST closing '%s'\n", name);
	pthread_mutex_lock(&fs->fdlock);
	for (i = 0; i < fs->fdlen; i++) {
		if (0 != strcmp(fs->fdlist[i].name, name))
			continue;
		fprintf(stderr, "FD_LIST closed '%s' at %d/%d bytes\n",
			name,
			(int)fs->fdlist[i].offset,
			(int)fs->fdlist[i].length);

		if (NULL != fs->fdlist[i].fd_tmp)
			fclose(fs->fdlist[i].fd_tmp);

		if (NULL != fs->fdlist[i].fd_main)
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

	fprintf(stderr, "FD_LIST removing '%s'\n", name);
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


int fd_list_request_next_offset(struct shfs *fs, const char *name)
{
	struct shfs_message m_alloc = {0};
	struct shfs_message *m = &m_alloc;
	size_t i;

	pthread_mutex_lock(&fs->fdlock);
	for (i = 0; i < fs->fdlen; i++) {
		if (0 != strcmp(fs->fdlist[i].name, name))
			continue;

		fprintf(stderr, "FD_LIST next %d/%d '%s'\n",
			(int)fs->fdlist[i].offset,
			(int)fs->fdlist[i].length,
			fs->fdlist[i].name);
		memset(m, 0, sizeof(*m));
		m->id = fs->id;
		m->key = fs->key;
		m->opcode = MESSAGE_READ;
		m->offset = (uint32_t)fs->fdlist[i].offset;
		strcpy(m->name, fs->fdlist[i].name);
		socket_sendto(fs->sock, m, sizeof(*m), fs->group_addr);

		pthread_mutex_unlock(&fs->fdlock);
		return 0;
	}
	pthread_mutex_unlock(&fs->fdlock);
	return -1;
}
