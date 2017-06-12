#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "shfs.h"


static void process_message(struct shfs *fs, struct shfs_message *m)
{
	struct shfs_file_op op_alloc;
	struct shfs_file_op *op = &op_alloc;

	memset(op, 0, sizeof(*op));

	if (m->opcode & MESSAGE_ISDIR)
		op->type |= FILE_OP_ISDIR;


	if (m->opcode & MESSAGE_CREATE) {
		op->type |= FILE_OP_REMOTE_CREATE;
		strcpy(op->name, m->name);
		queue_enqueue(fs->queue_fs, op);
		return;
	}


	if (m->opcode & MESSAGE_WRITE) {
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
	}


	if (m->opcode & MESSAGE_DELETE) {
		op->type |= FILE_OP_REMOTE_DELETE;
		strcpy(op->name, m->name);
		queue_enqueue(fs->queue_fs, op);
		return;
	}


	if (m->opcode & MESSAGE_RENAME) {
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
			"[%s:%d] <- [%s:%d] recv %d bytes "
			"(id, key) = (0x%x, 0x%x)\n",
			shost, sport, phost, pport, s, m->id, m->key);


		if (MESSAGE_PING == m->key) {
			m->key = MESSAGE_PONG;
			socket_sendto(fs->sock, m, s, fs->peer_addr);
			continue;
		}
		if (MESSAGE_PONG == m->key) {
			printf("[%s:%d] got pong\n",
				phost, pport);
			continue;
		}

		process_message(fs, m);
	}

	return NULL;
}