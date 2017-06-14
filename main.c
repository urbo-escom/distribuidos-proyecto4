#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <pthread.h>

#include "shfs.h"
#include "file.h"


int main(int argc, char **argv)
{
	char kazaa_name[1024];
	struct shfs *fs = NULL;
	pthread_t thread_recv;
	pthread_t thread_send;
	pthread_t thread_monitor;
	pthread_t thread_fs;

	fs = calloc(1, sizeof(*fs));
	if (NULL == fs) {
		perror("calloc");
		return EXIT_FAILURE;
	}
	srand(time(NULL));

	memset(fs, 0, sizeof(*fs));
	if (argc < 5) {
		fprintf(stderr, "usage: %s KAZAA_DIR TRASH_DIR "
				"GROUP PORT [OPTIONS]...\n",
			argv[0]);
		free(fs);
		return EXIT_FAILURE;
	}
	{
		int i;
		for (i = 5; i < argc; i++) {
			if (0 == strcmp("--host", argv[i])) {
				if (NULL == argv[i + 1]) {
					free(fs);
					fprintf(stderr, "Missing host arg\n");
					return EXIT_FAILURE;
				}
				fs->host = argv[i + 1];
				i++;
			}
		}
	}
	fs->maindir  = argv[1];
	fs->trashdir = argv[2];
	fs->tmpdir   = (sprintf(kazaa_name, "%s.bak", argv[1]), kazaa_name);
	fs->group    = argv[3];
	fs->port     = atoi(argv[4]);
	fs->key      = 0xdeadbeef;
	fs->id       = (uint32_t)rand();


	if (!file_exists(fs->maindir) || !file_isdir(fs->maindir)) {
		fprintf(stderr, "%s: is not a directory\n", fs->maindir);
		free(fs);
		return EXIT_FAILURE;
	}


	if (!file_exists(fs->trashdir) || !file_isdir(fs->trashdir)) {
		fprintf(stderr, "%s: is not a directory\n", fs->trashdir);
		free(fs);
		return EXIT_FAILURE;
	}


	file_mkdir(fs->tmpdir);
	if (!file_exists(fs->tmpdir) || !file_isdir(fs->tmpdir)) {
		fprintf(stderr, "%s: is not a directory\n", fs->tmpdir);
		free(fs);
		return EXIT_FAILURE;
	}


	socket_init();
	socket_addr_create(&fs->self_addr);
	socket_addr_create(&fs->peer_addr);
	socket_addr_create(&fs->group_addr);

	socket_udp_bind(&fs->sock, fs->self_addr, fs->host, fs->port);
	socket_addr_set_ip(fs->group_addr, fs->group);
	socket_addr_set_port(fs->group_addr, fs->port);
	socket_group_join(fs->sock, fs->group_addr);
	socket_recv_timeout_ms(fs->sock, 5000);
	socket_settimetolive(fs->sock, 10);


	fs->monitor = fnotify_open(fs->maindir);


	fs->queue_send = queue_create(sizeof(struct shfs_file_op));
	fs->queue_fs   = queue_create(sizeof(struct shfs_file_op));


	pthread_mutex_init(&fs->fdlock, NULL);


	fprintf(stderr, "KAZAA_DIR = \"%s\"\n", fs->maindir);
	fprintf(stderr, "TRASH_DIR = \"%s\"\n", fs->trashdir);
	fprintf(stderr, "TMP_DIR   = \"%s\"\n", fs->tmpdir);
	fprintf(stderr, "GROUP     = \"%s\"\n", fs->group);
	fprintf(stderr, "PORT      = %d\n", fs->port);


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
