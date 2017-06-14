#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include <sys/stat.h>

#include "shfs.h"
#include "file.h"


void fd_list_open(struct shfs *fs, const char *name, size_t length)
{
	char tmp_fname[1024];
	char main_fname[1024];
	size_t i;

	if (sizeof(fs->fdlist)/sizeof(fs->fdlist[0]) <= fs->fdlen)
		return;

	pthread_mutex_lock(&fs->fdlock);

	sprintf(tmp_fname, "%s/%s", fs->tmpdir, name);
	sprintf(main_fname, "%s/%s", fs->maindir, name);

	for (i = 0; i < fs->fdlen; i++)
		if (0 == strcmp(fs->fdlist[i].name, name)) {
			pthread_mutex_unlock(&fs->fdlock);
			return;
		}

	strcpy(fs->fdlist[i].name, name);
	fs->fdlist[i].fd_tmp = fopen(tmp_fname, "wb");
	fs->fdlist[i].fd_main = fopen(main_fname, "wb");
	fs->fdlist[i].offset = 0;
	fs->fdlist[i].length = length;
	fs->fdlist[i].lastrecv = time(NULL);
	fs->fdlen++;

	pthread_mutex_unlock(&fs->fdlock);
}


void fd_list_write(struct shfs *fs, const char *name,
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

		fprintf(stderr, "offset+len %d+%d\n", (int)offset, (int)len);
		fs->fdlist[i].offset += len;
		fs->fdlist[i].lastrecv = time(NULL);
		pthread_mutex_unlock(&fs->fdlock);
		return;
	}
	pthread_mutex_unlock(&fs->fdlock);
	fprintf(stderr, "finish writing to fdlist\n");
}


void fd_list_close(struct shfs *fs, const char *name)
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
		return;
	}
	pthread_mutex_unlock(&fs->fdlock);
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


void fd_list_remove(struct shfs *fs, const char *name)
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
		return;
	for (j = i + 1; j < fs->fdlen; i++, j++)
		memcpy(&fs->fdlist[i], &fs->fdlist[j], sizeof(fs->fdlist[i]));
	fs->fdlen--;
	pthread_mutex_unlock(&fs->fdlock);
}


void fs_remote_read(struct shfs *fs, struct shfs_file_op *op)
{
	struct shfs_message m_alloc = {0};
	struct shfs_message *m = &m_alloc;
	char tmp_fname[1024];
	FILE* fd;
	size_t len;

	fprintf(stderr, "REMOTE read %s [%s]\n",
		op->name,
		op->type & FILE_OP_ISDIR ? "dir":"file"
	);
	if (op->type & FILE_OP_ISDIR)
		return;

	sprintf(tmp_fname, "%s/%s", fs->tmpdir, op->name);
	fd = fopen(tmp_fname, "rb");
	if (NULL == fd) {
		perror(tmp_fname);
		return;
	}
	if (0 != fseek(fd, 0, op->offset)) {
		perror(tmp_fname);
		return;
	}

	m->id = fs->id;
	m->key = fs->key;
	m->opcode = MESSAGE_WRITE;
	len = fread(m->data, 1, sizeof(m->data), fd);
	m->count  = len;
	m->offset = op->offset;
	socket_sendto(fs->sock, m, sizeof(*m), fs->group_addr);
}


void fs_remote_create(struct shfs *fs, struct shfs_file_op *op)
{
	char tmp_fname[1024];
	char main_fname[1024];
	char trash_fname[1024];

	fprintf(stderr, "REMOTE create %s [%s]\n",
		op->name,
		op->type & FILE_OP_ISDIR ? "dir":"file"
	);

	sprintf(tmp_fname, "%s/%s", fs->tmpdir, op->name);
	sprintf(main_fname, "%s/%s", fs->maindir, op->name);
	sprintf(trash_fname, "%s/%s", fs->trashdir, op->name);

	if (op->type & FILE_OP_ISDIR) {
		if (!file_exists(tmp_fname)) file_mkdir(tmp_fname);
		if (!file_exists(main_fname)) file_mkdir(main_fname);
		if (!file_exists(trash_fname)) file_mkdir(trash_fname);
	} else {
		fd_list_open(fs, op->name, op->length);
	}
}


void fs_remote_write(struct shfs *fs, struct shfs_file_op *op)
{
	fprintf(stderr, "REMOTE write %s %d+%d/%d [%s]\n",
		op->name,
		(int)op->offset,
		(int)op->count,
		(int)op->length,
		op->type & FILE_OP_ISDIR ? "dir":"file"
	);
	if (op->type & FILE_OP_ISDIR)
		return;

	if (op->length <= op->offset || !op->count) {
		fd_list_close(fs, op->name);
		return;
	}

	fd_list_write(fs, op->name, op->offset, op->data, op->count);
}


void fs_remote_delete(struct shfs *fs, struct shfs_file_op *op)
{
	char tmp_fname[1024];
	char main_fname[1024];
	char trash_fname[1024];

	fprintf(stderr, "REMOTE delete %s [%s]\n",
		op->name,
		op->type & FILE_OP_ISDIR ? "dir":"file"
	);

	sprintf(tmp_fname, "%s/%s", fs->tmpdir, op->name);
	sprintf(main_fname, "%s/%s", fs->maindir, op->name);
	sprintf(trash_fname, "%s/%s", fs->trashdir, op->name);

	if (file_exists(main_fname)) file_rm(main_fname);
	if (file_exists(trash_fname)) file_rm(trash_fname);
	if (file_exists(tmp_fname))
		file_mv(tmp_fname, trash_fname);
}


void fs_remote_rename(struct shfs *fs, struct shfs_file_op *op)
{
	char old_tmpdir_fname[1024];
	char new_tmpdir_fname[1024];
	char old_maindir_fname[1024];
	char new_maindir_fname[1024];

	fprintf(stderr, "REMOTE rename %s -> %s [%s]\n",
		op->name,
		op->toname,
		op->type & FILE_OP_ISDIR ? "dir":"file"
	);

	sprintf(old_tmpdir_fname, "%s/%s", fs->tmpdir, op->name);
	sprintf(new_tmpdir_fname, "%s/%s", fs->tmpdir, op->toname);
	sprintf(old_maindir_fname, "%s/%s", fs->maindir, op->name);
	sprintf(new_maindir_fname, "%s/%s", fs->maindir, op->toname);

	if (file_exists(new_tmpdir_fname)) file_rm(new_tmpdir_fname);
	if (file_exists(new_maindir_fname)) file_rm(new_maindir_fname);
	file_mv(old_tmpdir_fname, new_tmpdir_fname);
	file_mv(old_maindir_fname, new_maindir_fname);
}


void fs_backup(struct shfs *fs, struct shfs_file_op *op)
{
	char tmp_fname[1024];
	char main_fname[1024];
	char trash_fname[1024];

	fprintf(stderr, "LOCAL backup %s [%s]\n",
		op->name,
		op->type & FILE_OP_ISDIR ? "dir":"file"
	);

	if (fd_list_has(fs, op->name)) {
		fd_list_remove(fs, op->name);
		fprintf(stderr, "LOCAL write %s finish [%s]\n",
			op->name,
			op->type & FILE_OP_ISDIR ? "dir":"file"
		);
		return;
	}

	sprintf(tmp_fname, "%s/%s", fs->tmpdir, op->name);
	sprintf(main_fname, "%s/%s", fs->maindir, op->name);
	sprintf(trash_fname, "%s/%s", fs->trashdir, op->name);

	if (file_isdir(main_fname) || (op->type & FILE_OP_ISDIR)) {
		if (!file_exists(tmp_fname))   file_mkdir(tmp_fname);
		if (!file_exists(trash_fname)) file_mkdir(trash_fname);
	} else if (!file_isdir(tmp_fname)) {
		file_cp(main_fname, tmp_fname);
	}

	queue_enqueue(fs->queue_send, op);
}


void fs_delete(struct shfs *fs, struct shfs_file_op *op)
{
	char tmp_fname[1024];
	char trash_fname[1024];

	fprintf(stderr, "LOCAL delete %s [%s]\n",
		op->name,
		op->type & FILE_OP_ISDIR ? "dir":"file"
	);

	sprintf(tmp_fname, "%s/%s", fs->tmpdir, op->name);
	sprintf(trash_fname, "%s/%s", fs->trashdir, op->name);

	if (!file_exists(tmp_fname))
		return;

	if (op->type & FILE_OP_ISDIR) {
		file_rm(tmp_fname);
	} else if (file_isdir(trash_fname) && file_exists(tmp_fname)) {
		file_rm(tmp_fname);
	} else {
		file_backup_mv(tmp_fname, trash_fname);
	}

	queue_enqueue(fs->queue_send, op);
}


void fs_rename(struct shfs *fs, struct shfs_file_op *op)
{
	char old_tmpdir_fname[1024];
	char new_tmpdir_fname[1024];
	char old_maindir_fname[1024];
	char new_maindir_fname[1024];

	fprintf(stderr, "LOCAL rename %s -> %s [%s]\n",
		op->name,
		op->toname,
		op->type & FILE_OP_ISDIR ? "dir":"file"
	);

	sprintf(old_tmpdir_fname, "%s/%s", fs->tmpdir, op->name);
	sprintf(new_tmpdir_fname, "%s/%s", fs->tmpdir, op->toname);
	sprintf(old_maindir_fname, "%s/%s", fs->maindir, op->name);
	sprintf(new_maindir_fname, "%s/%s", fs->maindir, op->toname);
	file_mv(old_tmpdir_fname, new_tmpdir_fname);
	file_mv(old_maindir_fname, new_maindir_fname);

	queue_enqueue(fs->queue_send, op);
}


void* fs_thread(void *param)
{
	struct shfs *fs = param;

	while (1) {
		struct shfs_file_op op_alloc;
		struct shfs_file_op *op = &op_alloc;
		int s;


		s = queue_dequeue(fs->queue_fs, op);
		if (-1 == s || 0 == op->type)
			return NULL;


		if (op->type & FILE_OP_REMOTE_READ) {
			fs_remote_read(fs, op);
			continue;
		}

		if (op->type & FILE_OP_REMOTE_CREATE) {
			fs_remote_create(fs, op);
			continue;
		}

		if (op->type & FILE_OP_REMOTE_WRITE) {
			fs_remote_write(fs, op);
			continue;
		}

		if (op->type & FILE_OP_REMOTE_DELETE) {
			fs_remote_delete(fs, op);
			continue;
		}

		if (op->type & FILE_OP_REMOTE_RENAME) {
			fs_remote_rename(fs, op);
			continue;
		}

		if (op->type & FILE_OP_BACKUP) {
			fs_backup(fs, op);
			continue;
		}

		if (op->type & FILE_OP_DELETE) {
			fs_delete(fs, op);
			continue;
		}

		if (op->type & FILE_OP_RENAME) {
			fs_rename(fs, op);
			continue;
		}

	}

	return NULL;
}
