#include <stdint.h>

#include "socket.h"
#include "fnotify.h"
#include "queue.h"

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


enum shfs_message_opcode {
	MESSAGE_CREATE = 0x01,
	MESSAGE_WRITE  = 0x02,
	MESSAGE_DELETE = 0x04,
	MESSAGE_RENAME = 0x08,
	MESSAGE_ISDIR  = 0x10,
	MESSAGE_PING   = 0xfe,
	MESSAGE_PONG   = 0xff
};


struct shfs_message {
	uint32_t id;
	uint32_t key;
	uint32_t opcode;
	uint32_t count;
	uint32_t offset;
	uint32_t length;
	char     name[MAX_NAME];
	char     data[MAX_BUFFER];
};


enum shfs_file_op_type {
	FILE_OP_REMOTE_CREATE = 0x01,
	FILE_OP_REMOTE_WRITE  = 0x02,
	FILE_OP_REMOTE_DELETE = 0x04,
	FILE_OP_REMOTE_RENAME = 0x08,
	FILE_OP_BACKUP        = 0x10,
	FILE_OP_DELETE        = 0x20,
	FILE_OP_RENAME        = 0x40,
	FILE_OP_ISDIR         = 0x80
};


struct shfs_file_op {
	enum shfs_file_op_type type;
	uint32_t               count;
	uint32_t               offset;
	uint32_t               length;
	char                   name[MAX_NAME];
	char                   toname[MAX_NAME];
	char                   data[MAX_BUFFER];
};


struct shfs {
	uint32_t     id;
	uint32_t     key;

	const char  *maindir;
	const char  *trashdir;
	const char  *tmpdir;

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
		FILE *fd_tmp;
		FILE *fd_main;
		char  name[1024];
	}            fdlist[1024];
	size_t       fdlen;
};
