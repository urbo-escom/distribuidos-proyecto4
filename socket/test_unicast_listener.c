#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socket.h"


void run(socket_fd *fd, socket_addr *self, socket_addr *peer)
{
	char remote_host[46];
	int  remote_port;
	char host[46];
	int  port;
	char buf[4096] = {0};
	int s;

	s = socket_recvfrom(fd, buf, sizeof(buf), peer);
	if (s < 0)
		return;

	socket_addr_get_ip(peer, remote_host, sizeof(remote_host));
	socket_addr_get_port(peer, &remote_port);
	printf("[%s:%d] -> ", remote_host, remote_port);

	socket_addr_get_ip(self, host, sizeof(host));
	socket_addr_get_port(self, &port);
	printf("[%s:%d]: '%*s'\n", host, port, s, buf);
}


int main(int argc, char **argv)
{
	char        bhost[46];
	int         bport;
	const char *host = "0.0.0.0";
	int         port = 0;
	socket_fd   *fd = NULL;
	socket_addr *self = NULL;
	socket_addr *peer = NULL;


	if (argc < 2) {
		fprintf(stderr, "usage: %s PORT\n", argv[0]);
		fprintf(stderr, "       %s HOST PORT\n", argv[0]);
		return EXIT_FAILURE;
	}
	if (2 == argc) {
		port = atoi(argv[1]);
	} else {
		host = argv[1];
		port = atoi(argv[2]);
	}


	socket_init();
	socket_addr_create(&self);
	socket_addr_create(&peer);
	socket_udp_bind(&fd, self, host, port);
	socket_addr_get_ip(self, bhost, sizeof(bhost));
	socket_addr_get_port(self, &bport);
	printf("# Listening on [%s:%d]\n", bhost, bport);

	while (1)
		run(fd, self, peer);


	return EXIT_SUCCESS;
}
