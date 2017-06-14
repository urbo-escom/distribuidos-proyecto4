#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <sys/stat.h>

#include "file.h"


int file_exists(const char *dir)
{
	struct stat buf;
	return 0 == stat(dir, &buf);
}


size_t file_size(const char *name)
{
	struct stat buf;
	if (0 != stat(name, &buf))
		return 0;
	return buf.st_size;
}


int file_isdir(const char *dir)
{
	struct stat buf;
	if (0 != stat(dir, &buf))
		return 0;
	return S_ISDIR(buf.st_mode);
}


int file_cp(const char *src, const char *dst)
{
	char str[1024];
	FILE* fd_dst = NULL;
	FILE* fd_src = NULL;
	int c;

	fd_dst = fopen(dst, "wb");
	if (NULL == fd_dst) {
		sprintf(str, "fopen(%s, write)", dst);
		perror(str);
		if (NULL != fd_dst) fclose(fd_dst);
		if (NULL != fd_src) fclose(fd_src);
		return -1;
	}

	fd_src = fopen(src, "rb");
	if (NULL == fd_src) {
		sprintf(str, "fopen(%s, read)", src);
		perror(str);
		if (NULL != fd_dst) fclose(fd_dst);
		if (NULL != fd_src) fclose(fd_src);
		return -1;
	}

	while (1) {
		c = fgetc(fd_src);
		if (feof(fd_src) || ferror(fd_src) || ferror(fd_dst))
			break;
		fputc(c, fd_dst);
	}

	fclose(fd_dst);
	fclose(fd_src);
	return -1;
}


int file_mv(const char *old, const char *new)
{
	char str[1024];
	if (-1 == rename(old, new)) {
		sprintf(str, "rename(%s, %s)", old, new);
		perror(str);
		return -1;
	}
	return 0;
}



int file_backup_mv(const char *old, const char *new)
{
	char backup[1024];

	strcpy(backup, new);
	while (file_exists(backup)) {
		char *t;
		t = strrchr(backup, '.');
		if (NULL == t) {
			strcat(backup, ".1");
		} else {
			char *end;
			long num;
			errno = 0;
			num = strtol(t + 1, &end, 0);
			if (errno || '\0' != *end) {
				strcat(backup, ".1");
			} else {
				num++;
				*(t + 1) = '\0';
				sprintf(t, ".%d", (int)num);
			}
		}
	}
	return file_mv(old, backup);
}


int file_rm(const char *name)
{
	char str[1024];
	if (-1 == remove(name)) {
		sprintf(str, "remove(%s)", name);
		perror(str);
		return -1;
	}
	return 0;
}


int file_mkdir(const char *dir)
{
	char str[1024];
#ifdef _WIN32
	if (-1 == mkdir(dir)) {
		sprintf(str, "mkdir(%s)", dir);
		perror(str);
		return -1;
	}
#else
	if (-1 == mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
		sprintf(str, "mkdir(%s)", dir);
		perror(str);
		return -1;
	}
#endif
	return 0;
}
