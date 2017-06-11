#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "socket.h"


int main(int argc, char **argv)
{
	const char *host = "0.0.0.0";
	int         port = 0;
	const char *msg  = "hello";
	socket_fd   *fd = NULL;
	socket_addr *peer = NULL;


	if (argc < 4) {
		fprintf(stderr, "usage: %s HOST PORT MESSAGE\n", argv[0]);
		return EXIT_FAILURE;
	}
	host = argv[1];
	port = atoi(argv[2]);
	msg  = argv[3];


	socket_init();

	socket_udp_create(&fd);

	socket_addr_create(&peer);
	socket_addr_set_ip(peer, host);
	socket_addr_set_port(peer, port);

	socket_sendto(fd, msg, strlen(msg), peer);

	return EXIT_SUCCESS;
}
