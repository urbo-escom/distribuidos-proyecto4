#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "shfs.h"


static void process_message(struct shfs *fs, struct shfs_message *m,
		const char *phost, int pport)
{
	struct shfs_file_op op_alloc;
	struct shfs_file_op *op = &op_alloc;

	memset(op, 0, sizeof(*op));

	if (m->type & MESSAGE_ISDIR)
		op->type |= FILE_OP_ISDIR;


	switch (m->opcode) {
	case MESSAGE_PING:
		m->opcode = MESSAGE_PONG;
		printf("RECV [%s:%d] got ping\n",
			phost, pport);
		socket_sendto(fs->sock, m, sizeof(*m), fs->peer_addr);
		return;


	case MESSAGE_PONG:
		printf("RECV [%s:%d] got pong\n",
			phost, pport);
		return;


	case MESSAGE_READ:
		op->type |= FILE_OP_REMOTE_READ;
		strcpy(op->name, m->name);
		queue_enqueue(fs->queue_fs, op);
		return;


	case MESSAGE_CREATE:
		op->type |= FILE_OP_REMOTE_CREATE;
		strcpy(op->name, m->name);
		queue_enqueue(fs->queue_fs, op);
		return;


	case MESSAGE_WRITE:
		op->type |= FILE_OP_REMOTE_WRITE;
		strcpy(op->name, m->name);
		op->count  = m->count;
		op->offset = m->offset;
		op->length = m->length;
		op->count  = m->count < sizeof(m->data) ?
			m->count:sizeof(m->data);
		memcpy(op->data, m->data, op->count);
		queue_enqueue(fs->queue_fs, op);
		return;


	case MESSAGE_DELETE:
		op->type |= FILE_OP_REMOTE_DELETE;
		strcpy(op->name, m->name);
		queue_enqueue(fs->queue_fs, op);
		return;


	case MESSAGE_RENAME:
		{
			char *t;
			op->type |= FILE_OP_REMOTE_RENAME;
			t = strchr(m->name, '|');
			if (NULL == t)
				return;
			*t = '\0';
			strcpy(op->name, m->name);
			strcpy(op->toname, t + 1);
			queue_enqueue(fs->queue_fs, op);
			return;
		}
	}

}


void fd_request_timeouted(struct shfs *fs)
{
	struct shfs_message m_alloc = {0};
	struct shfs_message *m = &m_alloc;
	size_t i;

	fprintf(stderr, "searching timeouts in %d files\n", (int)fs->fdlen);
	pthread_mutex_lock(&fs->fdlock);
	for (i = 0; i < fs->fdlen; i++) {
		/*
		int now;
		int last;
			now = time(NULL);
			last = fs->fdlist[i].lastrecv;
			fprintf(stderr, "resending offset %d %s\n",
				(int)fs->fdlist[i].offset,
				fs->fdlist[i].name);
		*/
			memset(m, 0, sizeof(*m));
			m->id = fs->id;
			m->key = fs->key;
			m->opcode = MESSAGE_READ;
			m->offset = (uint32_t)fs->fdlist[i].offset;
			strcpy(m->name, fs->fdlist[i].name);
			socket_sendto(fs->sock, m, sizeof(*m), fs->group_addr);
	}
	pthread_mutex_unlock(&fs->fdlock);
}


void* recv_thread(void *param)
{
	struct shfs *fs = param;

	while (1) {
		struct shfs_message m_alloc = {0};
		struct shfs_message *m = &m_alloc;
		char phost[46] = {0};
		char shost[46] = {0};
		int  pport = 0;
		int  sport = 0;
		int s;

		s = socket_recvfrom(fs->sock, m, sizeof(*m), fs->peer_addr);
		if (-1 == s && EAGAIN == errno) {
			fd_request_timeouted(fs);
			continue;
		}
		if (s <= 0)
			continue;


		if (m->id == fs->id) {
			/*
			fprintf(stderr, "id is equal to ours %08x == %08x\n",
				fs->id, m->id);
			*/
			continue;
		}
		if (m->key != fs->key) {
			/*
			fprintf(stderr, "key mismatch %08x != %08x\n",
				fs->key, m->key);
			*/
			continue;
		}


		socket_addr_get_ip(fs->peer_addr, phost, sizeof(phost));
		socket_addr_get_ip(fs->self_addr, shost, sizeof(shost));
		socket_addr_get_port(fs->peer_addr, &pport);
		socket_addr_get_port(fs->self_addr, &sport);
		fprintf(stderr,
			"RECV [%s:%d] <- [%s:%d] recv %d bytes "
			"(id, key) = (0x%x, 0x%x)\n",
			shost, sport, phost, pport, s, m->id, m->key);

		process_message(fs, m, phost, pport);
	}

	return NULL;
}
