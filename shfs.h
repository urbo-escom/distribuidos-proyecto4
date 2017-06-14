#include <stdint.h>
#include <pthread.h>

#include "socket.h"
#include "fnotify.h"
#include "queue.h"
#include "file.h"

#ifndef MAX_NAME
#define MAX_NAME 512
#endif

#ifndef MAX_BUFFER
#define MAX_BUFFER 512
#endif


extern void* recv_thread(void*);
extern void* send_thread(void*);
extern void* monitor_thread(void*);
extern void* fs_thread(void*);


enum shfs_message_type {
	MESSAGE_ISDIR = 0x01
};


enum shfs_message_opcode {
	MESSAGE_PING   = 0x00,
	MESSAGE_PONG   = 0x01,
	MESSAGE_READ   = 0x02,
	MESSAGE_CREATE = 0x03,
	MESSAGE_WRITE  = 0x04,
	MESSAGE_DELETE = 0x05,
	MESSAGE_RENAME = 0x06
};


struct shfs_message {
	uint32_t id;
	uint32_t key;
	uint32_t type;
	uint32_t opcode;
	uint32_t count;
	uint32_t offset;
	uint32_t length;
	char     name[MAX_NAME];
	char     data[MAX_BUFFER];
};


enum shfs_file_op_type {
	FILE_OP_REMOTE_READ   = 0x001,
	FILE_OP_REMOTE_CREATE = 0x002,
	FILE_OP_REMOTE_WRITE  = 0x004,
	FILE_OP_REMOTE_DELETE = 0x008,
	FILE_OP_REMOTE_RENAME = 0x010,
	FILE_OP_BACKUP        = 0x020,
	FILE_OP_DELETE        = 0x040,
	FILE_OP_RENAME        = 0x080,
	FILE_OP_ISDIR         = 0x100
};


struct shfs_file_op {
	enum shfs_file_op_type type;
	uint32_t               count;
	uint32_t               offset;
	uint32_t               length;
	char                   name[MAX_NAME];
	char                   toname[MAX_NAME];
	char                   data[MAX_BUFFER];
	char                   host[46];
	int                    port;
};


struct shfs {
	uint32_t     id;  /* per-client *unique* id */
	uint32_t     key; /* per-app    *unique* id */

	const char  *maindir;
	const char  *trashdir;
	const char  *tmpdir;
	int          clean;

	const char  *host;
	const char  *group;
	int          port;

	socket_fd   *sock;
	socket_addr *self_addr;
	socket_addr *peer_addr;
	socket_addr *group_addr;

	fnotify     *monitor;
	queue       *queue_send;
	queue       *queue_fs;

	struct {
		FILE   *fd_tmp;
		FILE   *fd_main;
		size_t  offset;
		size_t  length;
		int     lastrecv;
		char    name[1024];
	}                *fdlist;
	size_t            fdsize;
	size_t            fdlen;
	pthread_mutex_t   fdlock;
};


extern int fd_list_open(struct shfs *fs, const char *name, size_t length);
extern int fd_list_write(struct shfs *fs, const char *name, size_t offset,
		const void *buf, size_t len);
extern int fd_list_close(struct shfs *fs, const char *name);
extern int fd_list_has(struct shfs *fs, const char *name);
extern int fd_list_remove(struct shfs *fs, const char *name);
extern int fd_list_request_next_offset(struct shfs *fs, const char *name);
