#include "_common.h"


int socket_setnonblocking(socket_fd *fd)
{
#ifdef _WIN32
	u_long t = 1;
	int s;

	assert(NULL != fd && "fd cannot be NULL");
	s = ioctlsocket(fd->sock, FIONBIO, &t);
	if (SOCKET_ERROR == s)
		return -1;

	return 0;
#else
	int flags;
	int s;

	assert(NULL != fd && "fd cannot be NULL");

	flags = fcntl(fd->sock, F_GETFL, 0);
	if (-1 == flags)
		return -1;

	flags |= O_NONBLOCK;
	s = fcntl(fd->sock, F_SETFL, flags);
	if (-1 == s)
		return -1;

	return 0;
#endif
}


int socket_setbroadcast(socket_fd *fd)
{
#ifdef _WIN32
	DWORD t = TRUE;
	int s;

	assert(NULL != fd && "fd cannot be NULL");
	s = setsockopt(fd->sock, SOL_SOCKET, SO_BROADCAST,
		(char*)&t, sizeof(t));
	if (SOCKET_ERROR == s)
		return -1;

	return 0;
#else
	int t = 1;
	int s;

	assert(NULL != fd && "fd cannot be NULL");
	s = setsockopt(fd->sock, SOL_SOCKET, SO_BROADCAST,
		&t, sizeof(t));
	if (-1 == s)
		return -1;

	return 0;
#endif
}


int socket_settimetolive(socket_fd *fd, int ttl)
{
#ifdef _WIN32
	DWORD t;
#else
	unsigned char t;
#endif
	int s;
	assert(NULL != fd && "fd cannot be NULL");

	t =
		ttl < 0 ? 1:
		255 < ttl ? 255:
		(unsigned char)ttl;

	s = setsockopt(fd->sock, IPPROTO_IP, IP_MULTICAST_TTL,
		(void*)&t, sizeof(t));
	if (SOCKET_ERROR == s)
		return -1;
	return 0;
}


int socket_group_join(socket_fd *fd, const socket_addr *addr)
{
	const struct sockaddr_in *addr4;
	struct ip_mreq m;
	int s;

	addr4 = (struct sockaddr_in*)&addr->addr;
	m.imr_multiaddr.s_addr = addr4->sin_addr.s_addr;
	m.imr_interface.s_addr = htonl(INADDR_ANY);

	s = setsockopt(fd->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(void*)&m, sizeof(m));
	if (SOCKET_ERROR == s)
		return -1;
	return 0;
}


int socket_group_leave(socket_fd *fd, const socket_addr *addr)
{
	struct sockaddr_in *addr4;
	struct ip_mreq m;
	int s;

	addr4 = (struct sockaddr_in*)&addr->addr;
	m.imr_multiaddr.s_addr = addr4->sin_addr.s_addr;
	m.imr_interface.s_addr = htonl(INADDR_ANY);

	s = setsockopt(fd->sock, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		(void*)&m, sizeof(m));
	if (SOCKET_ERROR == s)
		return -1;
	return 0;
}


int socket_recv_timeout_ms(socket_fd *fd, int msec)
{
#ifdef _WIN32
	DWORD t;
	int s;

	assert(NULL != fd && "fd cannot be NULL");
	t = msec < 0 ? 0:msec;
	s = setsockopt(fd->sock, SOL_SOCKET, SO_RCVTIMEO,
		(char*)&t, sizeof(t));
	if (SOCKET_ERROR == s)
		return -1;

	return 0;
#else
	int s;
	struct timeval tv;
	socklen_t      tvlen = sizeof(tv);

	assert(NULL != fd && "fd cannot be NULL");

	if (msec < 0)
		msec = 0;

	tv.tv_sec = msec/1000;
	tv.tv_usec = (msec%1000)*1000;
	s = setsockopt(fd->sock, SOL_SOCKET, SO_RCVTIMEO, &tv, tvlen);
	if (-1 == s)
		return -1;

	return 0;
#endif
}


int socket_send_timeout_ms(socket_fd *fd, int msec)
{
#ifdef _WIN32
	DWORD t;
	int s;

	assert(NULL != fd && "fd cannot be NULL");
	t = msec < 0 ? 0:msec;
	s = setsockopt(fd->sock, SOL_SOCKET, SO_SNDTIMEO,
		(char*)&t, sizeof(t));
	if (SOCKET_ERROR == s)
		return -1;

	return 0;
#else
	int s;
	struct timeval tv;
	socklen_t      tvlen = sizeof(tv);

	assert(NULL != fd && "fd cannot be NULL");

	if (msec < 0)
		msec = 0;

	tv.tv_sec = msec/1000;
	tv.tv_usec = (msec%1000)*1000;
	s = setsockopt(fd->sock, SOL_SOCKET, SO_SNDTIMEO, &tv, tvlen);
	if (-1 == s)
		return -1;

	return 0;
#endif
}
