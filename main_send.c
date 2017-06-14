#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shfs.h"


void send_backup(struct shfs *fs, struct shfs_file_op *op,
		struct shfs_message *m)
{
	char tmp_fname[1024];

	sprintf(tmp_fname, "%s/%s", fs->tmpdir, op->name);

	m->opcode = MESSAGE_CREATE;
	strcpy(m->name, op->name);
	if (file_isdir(tmp_fname)) {
		m->type   = MESSAGE_ISDIR;
		m->length = 0;
		fprintf(stderr, "SEND dir %d bytes '%s'\n",
			(int)m->length, op->name);
		socket_sendto(fs->sock, m, sizeof(*m), fs->group_addr);
	} else {
		m->type   = 0;
		m->length = file_size(tmp_fname);
		fprintf(stderr, "SEND file %d bytes '%s'\n",
			(int)m->length, op->name);
		socket_sendto(fs->sock, m, sizeof(*m), fs->group_addr);
	}
}


void send_delete(struct shfs *fs, struct shfs_file_op *op,
		struct shfs_message *m)
{
	m->opcode = MESSAGE_DELETE;
	strcpy(m->name, op->name);
	socket_sendto(fs->sock, m, sizeof(*m), fs->group_addr);
}


void send_rename(struct shfs *fs, struct shfs_file_op *op,
		struct shfs_message *m)
{
	m->opcode = MESSAGE_RENAME;
	sprintf(m->name, "%s|%s", op->name, op->toname);
	socket_sendto(fs->sock, m, sizeof(*m), fs->group_addr);
}


void* send_thread(void *param)
{
	struct shfs *fs = param;

	while (1) {
		struct shfs_file_op op_alloc = {0};
		struct shfs_file_op *op = &op_alloc;
		struct shfs_message m_alloc = {0};
		struct shfs_message *m = &m_alloc;
		char phost[46] = {0};
		char shost[46] = {0};
		int  pport = 0;
		int  sport = 0;
		int s;


		s = queue_dequeue(fs->queue_send, op);
		if (-1 == s || 0 == op->type)
			return NULL;


		m->id  = fs->id;
		m->key = fs->key;


		if (op->type & FILE_OP_BACKUP) {
			send_backup(fs, op, m);
			goto print;
		}


		if (op->type & FILE_OP_DELETE) {
			send_delete(fs, op, m);
			goto print;
		}


		if (op->type & FILE_OP_RENAME) {
			send_rename(fs, op, m);
			goto print;
		}

		continue;

	print:
		socket_addr_get_ip(fs->group_addr, phost, sizeof(phost));
		socket_addr_get_ip(fs->self_addr, shost, sizeof(shost));
		socket_addr_get_port(fs->group_addr, &pport);
		socket_addr_get_port(fs->self_addr, &sport);
		fprintf(stderr,
			"SEND [%s:%d] -> [%s:%d] sent "
			"(id, key) = (0x%x, 0x%x)\n",
			shost, sport, phost, pport, m->id, m->key);
	}

	return NULL;
}
