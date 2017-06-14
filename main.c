#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pthread.h>

#include "shfs.h"


#ifndef FD_LIST_SIZE
#define FD_LIST_SIZE (1024)
#endif


const char *progname = "kazaa-fs";


void print_options(struct shfs *fs)
{
	fprintf(stderr, "KAZAA_DIR = \"%s\"\n", fs->maindir);
	fprintf(stderr, "TRASH_DIR = \"%s\"\n", fs->trashdir);
	fprintf(stderr, "TMP_DIR   = \"%s\"\n", fs->tmpdir);
	fprintf(stderr, "HOST      = \"%s\"\n", fs->host ? fs->host:"0.0.0.0");
	fprintf(stderr, "GROUP     = \"%s\"\n", fs->group);
	fprintf(stderr, "PORT      = %d\n", fs->port);
	fprintf(stderr, "ID        = 0x%08x\n", fs->id);
	fprintf(stderr, "KEY       = 0x%08x\n", fs->key);
}


void print_help(struct shfs *fs)
{
	fprintf(stderr, "USAGE: %s [OPTIONS]... KAZAA_DIR TRASH_DIR\n",
		progname);
	fprintf(stderr, "\n");
	fprintf(stderr, "OPTIONS\n");

	fprintf(stderr, "\t--help\n");
	fprintf(stderr, "\t\tPrint this help\n\n");

	fprintf(stderr, "\t--clean\n");
	fprintf(stderr, "\t\tClean all directories before starting\n\n");

	fprintf(stderr, "\t--kazaa DIR\n");
	fprintf(stderr, "\t\tSet DIR as KAZAA_DIR\n\n");

	fprintf(stderr, "\t--trash DIR\n");
	fprintf(stderr, "\t\tSet DIR as TRASH_DIR\n\n");

	fprintf(stderr, "\t--tmp DIR\n");
	fprintf(stderr, "\t\tSet DIR as TMP_DIR\n\n");

	fprintf(stderr, "\t--host IP\n");
	fprintf(stderr, "\t\tSet IP as HOST (for binding)\n\n");

	fprintf(stderr, "\t--group IP\n");
	fprintf(stderr, "\t\tSet IP as GROUP (for multicast)\n\n");

	fprintf(stderr, "\t--port NUM\n");
	fprintf(stderr, "\t\tSet NUM as PORT\n\n");

	fprintf(stderr, "\t--id NUM\n");
	fprintf(stderr, "\t\tSet NUM as ID (MUST be unique per-client)\n\n");

	fprintf(stderr, "\t--key NUM\n");
	fprintf(stderr, "\t\tSet NUM as KEY (MUST be unique per-app)\n\n");

	print_options(fs);
}


int parse_args(struct shfs *fs, int argc, char **argv)
{
	int i;
	int can_exit = 0;
	int parsed_maindir = 0;
	int parsed_trashdir = 0;


	for (i = 0; i < argc; i++) {
		if (0 == strcmp("--help", argv[i])) {
			can_exit = 1;
			continue;
		}

		if (0 == strcmp("--clean", argv[i])) {
			fs->clean = 1;
			continue;
		}

		if (0 == strcmp("--kazaa", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(fs);
				fprintf(stderr, "Missing --kazaa arg\n");
				exit(EXIT_FAILURE);
			}
			fs->maindir = argv[i + 1];
			i++;
			continue;
		}

		if (0 == strcmp("--trash", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(fs);
				fprintf(stderr, "Missing --trash arg\n");
				exit(EXIT_FAILURE);
			}
			fs->trashdir = argv[i + 1];
			i++;
			continue;
		}

		if (0 == strcmp("--tmp", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(fs);
				fprintf(stderr, "Missing --tmp arg\n");
				exit(EXIT_FAILURE);
			}
			fs->tmpdir = argv[i + 1];
			i++;
			continue;
		}

		if (0 == strcmp("--host", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(fs);
				fprintf(stderr, "Missing --host arg\n");
				exit(EXIT_FAILURE);
			}
			fs->host = argv[i + 1];
			i++;
			continue;
		}

		if (0 == strcmp("--group", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(fs);
				fprintf(stderr, "Missing --group arg\n");
				exit(EXIT_FAILURE);
			}
			fs->group = argv[i + 1];
			i++;
			continue;
		}

		if (0 == strcmp("--port", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(fs);
				fprintf(stderr, "Missing --port arg\n");
				exit(EXIT_FAILURE);
			}
			fs->port = strtol(argv[i + 1], NULL, 0);
			i++;
			continue;
		}

		if (0 == strcmp("--id", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(fs);
				fprintf(stderr, "Missing --id arg\n");
				exit(EXIT_FAILURE);
			}
			fs->id = strtol(argv[i + 1], NULL, 0);
			i++;
			continue;
		}

		if (0 == strcmp("--key", argv[i])) {
			if (NULL == argv[i + 1]) {
				print_help(fs);
				fprintf(stderr, "Missing --key arg\n");
				exit(EXIT_FAILURE);
			}
			fs->key = strtol(argv[i + 1], NULL, 0);
			i++;
			continue;
		}

		if (!parsed_maindir) {
			fs->maindir = argv[i];
			parsed_maindir = 1;
		}

		if (!parsed_trashdir) {
			fs->trashdir = argv[i];
			parsed_trashdir = 1;
		}

	}

	if (can_exit) {
		print_help(fs);
		exit(EXIT_FAILURE);
	}

	return 0;
}


int main(int argc, char **argv)
{
	struct shfs *fs = NULL;
	pthread_t thread_recv;
	pthread_t thread_send;
	pthread_t thread_monitor;
	pthread_t thread_fs;
	int s;


#ifdef _WIN32
	if (NULL != getenv("MSYSTEM") || NULL != getenv("CYGWIN")) {
		setvbuf(stdout, 0, _IONBF, 0);
		setvbuf(stderr, 0, _IONBF, 0);
	}
#endif
	srand(time(NULL));


	fs = calloc(1, sizeof(*fs));
	if (NULL == fs) {
		perror("calloc");
		return EXIT_FAILURE;
	}
	{
		char buf0[1024];
		char buf1[1024];
		fs->maindir  = "kazaa";
		fs->tmpdir   = (sprintf(buf0, "%s.bak", fs->maindir), buf0);
		fs->trashdir = (sprintf(buf1, "%s.trash", fs->maindir), buf1);
		fs->group    = "224.0.0.1";
		fs->port     = 7000;
		fs->key      = 0xdeadbeef;
		fs->id       = (uint32_t)rand();
	}


	fs->fdlist = calloc(FD_LIST_SIZE, sizeof(*fs->fdlist));
	if (NULL == fs->fdlist) {
		perror("calloc");
		return EXIT_FAILURE;
	}
	fs->fdsize = FD_LIST_SIZE;
	fs->fdlen  = 0;
	pthread_mutex_init(&fs->fdlock, NULL);


	progname = argv[0];
	if (NULL != strrchr(progname, '\\'))
		progname = strrchr(progname, '\\') + 1;
	if (NULL != strrchr(progname, '/'))
		progname = strrchr(progname, '/') + 1;
	parse_args(fs, argc - 1, argv + 1);


	if (fs->clean) {
		if (file_exists(fs->tmpdir)) file_rm(fs->tmpdir);
		if (file_exists(fs->maindir)) file_rm(fs->maindir);
		if (file_exists(fs->trashdir)) file_rm(fs->trashdir);
	}

	if (!file_isdir(fs->tmpdir)) file_mkdir(fs->tmpdir);
	if (!file_isdir(fs->maindir)) file_mkdir(fs->maindir);
	if (!file_isdir(fs->trashdir)) file_mkdir(fs->trashdir);


	if (!file_isdir(fs->maindir)) {
		fprintf(stderr, "%s: is not a directory\n", fs->maindir);
		return EXIT_FAILURE;
	}

	if (!file_isdir(fs->trashdir)) {
		fprintf(stderr, "%s: is not a directory\n", fs->trashdir);
		return EXIT_FAILURE;
	}

	if (!file_isdir(fs->tmpdir)) {
		fprintf(stderr, "%s: is not a directory\n", fs->tmpdir);
		return EXIT_FAILURE;
	}


	socket_init();
	socket_addr_create(&fs->self_addr);
	socket_addr_create(&fs->peer_addr);
	socket_addr_create(&fs->group_addr);

	s = socket_udp_bind(&fs->sock, fs->self_addr, fs->host, fs->port);
	if (-1 == s) {
		print_options(fs);
		fprintf(stderr, "Could not bind to [%s:%d]\n",
			fs->host ? fs->host:"0.0.0.0", fs->port);
		exit(EXIT_FAILURE);
	}
	socket_recv_timeout_ms(fs->sock, 5000);
	socket_settimetolive(fs->sock, 10);

	socket_addr_set_ip(fs->group_addr, fs->group);
	socket_addr_set_port(fs->group_addr, fs->port);
	s = socket_group_join(fs->sock, fs->group_addr);
	if (-1 == s) {
		print_options(fs);
		fprintf(stderr, "Could not join to [%s:%d]\n",
			fs->group ? fs->group:"0.0.0.0", fs->port);
		exit(EXIT_FAILURE);
	}


	fs->monitor = fnotify_open(fs->maindir);


	fs->queue_send = queue_create(sizeof(struct shfs_file_op));
	fs->queue_fs   = queue_create(sizeof(struct shfs_file_op));


	print_options(fs);


	pthread_create(&thread_recv,    NULL, recv_thread,    fs);
	pthread_create(&thread_send,    NULL, send_thread,    fs);
	pthread_create(&thread_monitor, NULL, monitor_thread, fs);
	pthread_create(&thread_fs,      NULL, fs_thread,      fs);
	pthread_join(thread_recv,    NULL);
	pthread_join(thread_send,    NULL);
	pthread_join(thread_monitor, NULL);
	pthread_join(thread_fs,      NULL);


	return EXIT_SUCCESS;
}
