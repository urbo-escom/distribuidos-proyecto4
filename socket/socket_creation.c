#include "_common.h"


int socket_udp_create(socket_fd **fd)
{
	socket_fd *p;

	p = malloc(sizeof(*p));
	if (NULL == p)
		return -1;
	p->sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (INVALID_SOCKET == p->sock) {
		preserving_error(free(p));
	}

	*fd = p;
	return 0;
}


int socket_close(socket_fd *fd)
{
	if (NULL == fd)
		return 0;
	if (SOCKET_ERROR == closesocket(fd->sock))
		return -1;
	free(fd);
	return 0;
}


void socket_get_handle(const socket_fd *fd, void *handle)
{
	if (NULL == handle)
		return;
	memcpy(handle, &fd->sock, sizeof(fd->sock));
}


void socket_set_handle(socket_fd *fd, const void *handle)
{
	if (NULL == handle)
		return;
	memcpy(&fd->sock, handle, sizeof(fd->sock));
}


int socket_bind(socket_fd *fd, const socket_addr *addr)
{
	int s;

	assert(NULL != addr && "fd cannot be NULL");
	assert(NULL != addr && "addr cannot be NULL");

	s = bind(fd->sock, (struct sockaddr*)&addr->addr, addr->addrlen);
	if (SOCKET_ERROR == s)
		return -1;

	return 0;
}


int socket_connect(socket_fd *fd, const socket_addr *addr)
{
	int s;

	assert(NULL != addr && "fd cannot be NULL");
	assert(NULL != addr && "addr cannot be NULL");

	s = connect(fd->sock, (struct sockaddr*)&addr->addr, addr->addrlen);
	if (SOCKET_ERROR == s)
		return -1;

	return 0;
}


int socket_getpeeraddr(const socket_fd *fd, socket_addr *addr)
{
	int s;

	assert(NULL != addr && "fd cannot be NULL");
	assert(NULL != addr && "addr cannot be NULL");

	addr->addrlen = sizeof(addr->addr);
	s = getpeername(fd->sock,
		(struct sockaddr*)&addr->addr, &addr->addrlen);
	if (SOCKET_ERROR == s)
		return -1;

	return 0;
}


int socket_getselfaddr(const socket_fd *fd, socket_addr *addr)
{
	int s;

	assert(NULL != addr && "fd cannot be NULL");
	assert(NULL != addr && "addr cannot be NULL");

	addr->addrlen = sizeof(addr->addr);
	s = getsockname(fd->sock,
		(struct sockaddr*)&addr->addr, &addr->addrlen);
	if (SOCKET_ERROR == s)
		return -1;

	return 0;
}


int socket_udp_bind(socket_fd **fd, socket_addr *addr,
		const char *ip, int port)
{
	struct addrinfo hints = {0};
	struct addrinfo *rp = NULL;
	struct addrinfo *r = NULL;
	socket_t sock = INVALID_SOCKET;
	const char *host;
	char serv[16];
	int s;

	assert(NULL != fd && "fd cannot be NULL");

	if (port < 0 || 65535 < port)
		return errno = EAGAIN, -1;

	host = ip;
	sprintf(serv, "%d", port);

	hints.ai_family   = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags    = AI_PASSIVE;

	s = getaddrinfo(host, serv, &hints, &r);
	if (0 != s)
		return -1;
	for (rp = r; NULL != rp; rp = rp->ai_next) {
		void *pp;

		sock = socket(
			rp->ai_family,
			rp->ai_socktype,
			rp->ai_protocol
		);
		if (INVALID_SOCKET == sock)
			continue;

		s = bind(sock, rp->ai_addr, rp->ai_addrlen);
		if (SOCKET_ERROR == s) {
			closesocket(sock), sock = INVALID_SOCKET;
			continue;
		}

		pp = malloc(sizeof(*fd));
		if (NULL == pp) {
			preserving_error((
				closesocket(sock),
				freeaddrinfo(r)
			));
			return -1;
		}
		*fd = pp;

		(*fd)->sock = sock;
		if (NULL != addr) {
			memcpy(&addr->addr, rp->ai_addr, rp->ai_addrlen);
			addr->addrlen = rp->ai_addrlen;
		}
		freeaddrinfo(r);
		return 0;
	}
	freeaddrinfo(r);

	return -1;
}
